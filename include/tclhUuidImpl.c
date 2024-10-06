/*
 * Copyright (c) 2021-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhUuid.h"

/*
 * Uuid: Tcl_Obj custom type
 * Implements custom Tcl_Obj wrapper for Uuid.
 */
static void DupUuidObj(Tcl_Obj *srcObj, Tcl_Obj *dstObj);
static void FreeUuidObj(Tcl_Obj *objP);
static void StringFromUuidObj(Tcl_Obj *objP);
static int  SetUuidObjFromAny(Tcl_Obj *objP);

static struct Tcl_ObjType gUuidVtbl = {
    "Tclh_Uuid",
    FreeUuidObj,
    DupUuidObj,
    StringFromUuidObj,
    NULL
};
TCLH_INLINE Tclh_UUID *IntrepGetUuid(Tcl_Obj *objP) {
    return (Tclh_UUID *) objP->internalRep.twoPtrValue.ptr1;
}
TCLH_INLINE void IntrepSetUuid(Tcl_Obj *objP, Tclh_UUID *value) {
    objP->internalRep.twoPtrValue.ptr1 = (void *) value;
}

Tclh_Bool Tclh_UuidIsObjIntrep (Tcl_Obj *objP) {
    return objP->typePtr == &gUuidVtbl;
}

static void DupUuidObj(Tcl_Obj *srcObj, Tcl_Obj *dstObj)
{
    Tclh_UUID *uuidP = (Tclh_UUID *) ckalloc(16);
    memcpy(uuidP, IntrepGetUuid(srcObj), 16);
    IntrepSetUuid(dstObj, uuidP);
    dstObj->typePtr = &gUuidVtbl;
}

static void FreeUuidObj(Tcl_Obj *objP)
{
    ckfree(IntrepGetUuid(objP));
    IntrepSetUuid(objP, NULL);
}

static void StringFromUuidObj(Tcl_Obj *objP)
{
#ifdef _WIN32
    UUID *uuidP = IntrepGetUuid(objP);
    unsigned char *uuidStr;
    Tcl_Size  len;
    if (UuidToStringA(uuidP, &uuidStr) != RPC_S_OK) {
        TCLH_PANIC("Out of memory stringifying UUID.");
    }
    len          = Tclh_strlen((char *)uuidStr);
    objP->bytes  = Tclh_strdupn((char *)uuidStr, len);
    objP->length = len;
    RpcStringFreeA(&uuidStr);
#elif defined(__APPLE__)
    char buf[100];
    size_t len;
    CFUUIDBytes *uuidP = IntrepGetUuid(objP);
    CFUUIDRef uuidRef  = CFUUIDCreateFromUUIDBytes(NULL, *uuidP);
    CFStringRef strRef = CFUUIDCreateString(NULL, uuidRef);
    if (!CFStringGetCString(strRef, buf, sizeof(buf), kCFStringEncodingUTF8)) {
        TCLH_PANIC("UUID string conversion failed.");
    }
    len         = strlen(buf);
    objP->bytes = ckalloc(len+1);
    memmove(objP->bytes, buf, len + 1);
    objP->length = len;         /* Not counting terminating \0 */
    CFRelease(strRef);
    CFRelease(uuidRef);
#else
    objP->bytes = ckalloc(37); /* Number of bytes for string rep */
    objP->length = 36;         /* Not counting terminating \0 */
    uuid_unparse_lower(IntrepGetUuid(objP)->bytes, objP->bytes);
#endif
}

static int  SetUuidObjFromAny(Tcl_Obj *objP)
{
    Tclh_UUID *uuidP;

    if (objP->typePtr == &gUuidVtbl)
        return TCL_OK;
    char buf[37];
    Tcl_Size len;
    const char *s = Tcl_GetStringFromObj(objP, &len);
    /* Accomodate Windows GUID style representation with curly braces */
    if (s[0] == '{' && len == 38 && s[37] == '}') {
        memcpy(buf, s+1, 36);
        buf[36] = '\0';
        s       = buf;
    }
    uuidP = ckalloc(sizeof(*uuidP));

#ifdef _WIN32

    RPC_STATUS rpcStatus = UuidFromStringA(s, uuidP);
    if (rpcStatus != RPC_S_OK) {
        ckfree(uuidP);
        return TCL_ERROR;
    }

#elif defined(__APPLE__)

    CFStringRef strRef =
        CFStringCreateWithCString(NULL, s, kCFStringEncodingUTF8);
    if (strRef == NULL)
        return TCL_ERROR;
    CFUUIDRef uuidRef = CFUUIDCreateFromString(NULL, strRef);
    *uuidP            = CFFUUIDGetUUIDBytes(uuidRef);

    CFRelease(uuidRef);
    CFRelease(strRef);

#else

    if (uuid_parse(s, uuidP->bytes) != 0) {
        ckfree(uuidP);
        return TCL_ERROR;
    }

#endif

    IntrepSetUuid(objP, uuidP);
    objP->typePtr = &gUuidVtbl;
    return TCL_OK;
}

Tcl_Obj *Tclh_UuidWrap (const Tclh_UUID *from)
{
    Tcl_Obj *objP;
    Tclh_UUID *uuidP;

    objP = Tcl_NewObj();
    Tcl_InvalidateStringRep(objP);
    uuidP = ckalloc(sizeof(*uuidP));
    memcpy(uuidP, from, sizeof(*uuidP));
    IntrepSetUuid(objP, uuidP);
    objP->typePtr = &gUuidVtbl;
    return objP;
}

Tclh_ReturnCode
Tclh_UuidUnwrap(Tcl_Interp *interp, Tcl_Obj *objP, Tclh_UUID *uuidP)
{
    if (SetUuidObjFromAny(objP) != TCL_OK) {
        return Tclh_ErrorInvalidValue(interp, objP, "Invalid UUID format.");
    }

    memcpy(uuidP, IntrepGetUuid(objP), sizeof(*uuidP));
    return TCL_OK;
}

Tcl_Obj *Tclh_UuidNewObj (Tcl_Interp *ip)
{
    Tcl_Obj *objP;
    Tclh_UUID *uuidP = ckalloc(sizeof(*uuidP));
#ifdef _WIN32

    if (UuidCreate(uuidP) != RPC_S_OK) {
        if (UuidCreateSequential(uuidP) != RPC_S_OK) {
            TCLH_PANIC("Unable to create UUID.");
        }
    }

#elif defined(__APPLE__)

    CFUUIDRef uuidRef = CFUUIDCreate(NULL);
    *uuidP            = CFUUIDGetUUIDBytes(uuidRef);
    CFRelease(uuidRef);

#else

    uuid_generate(uuidP->bytes);

#endif /* _WIN32 */

    objP = Tcl_NewObj();
    Tcl_InvalidateStringRep(objP);
    IntrepSetUuid(objP, uuidP);
    objP->typePtr = &gUuidVtbl;
    return objP;
}