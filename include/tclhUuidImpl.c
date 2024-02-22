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
#else
    objP->bytes = ckalloc(37); /* Number of bytes for string rep */
    objP->length = 36;         /* Not counting terminating \0 */
    uuid_unparse_lower(IntrepGetUuid(objP), objP->bytes);
#endif
}

static int  SetUuidObjFromAny(Tcl_Obj *objP)
{
    Tclh_UUID *uuidP;

    if (objP->typePtr == &gUuidVtbl)
        return TCL_OK;
    uuidP = ckalloc(sizeof(*uuidP));
#ifdef _WIN32
    RPC_STATUS rpcStatus = UuidFromStringA((unsigned char *) Tcl_GetString(objP), 
                                           uuidP);
    if (rpcStatus != RPC_S_OK) {
        ckfree(uuidP);
        return TCL_ERROR;
    }
#else
    if (uuid_parse(Tcl_GetString(objP), uuidP) != 0) {
        ckfree(uuidP);
        return TCL_ERROR;
    }
#endif /* _WIN32 */

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
#else
    uuid_generate(uuidP);
#endif /* _WIN32 */

    objP = Tcl_NewObj();
    Tcl_InvalidateStringRep(objP);
    IntrepSetUuid(objP, uuidP);
    objP->typePtr = &gUuidVtbl;
    return objP;
}