/*
 * Copyright (c) 2022-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhBase.h"

typedef struct TclhPointerRegistry TclhPointerRegistry;
struct Tclh_LibContext {
    Tcl_Interp *interp;
    TclhPointerRegistry *pointerRegistryP; /* PointerLib */
    Tcl_HashTable *atomRegistryP;          /* AtomLib */
#if defined(_WIN32)
    Tcl_Encoding encWinChar;               /* EncodingLib */
#endif
};

char *TclhPrintAddress(const void *address, char *buf, int buflen);

#ifndef TCLH_LIB_CONTEXT_NAME
/* This will be shared for all extensions if embedder has not defined it */
# define TCLH_LIB_CONTEXT_NAME "TclhLibContext"
#endif

static void
TclhCleanupLib(ClientData clientData, Tcl_Interp *interp)
{
    Tclh_LibContext *ctxP = (Tclh_LibContext *)clientData;
    /*
     * Note the CONTENT of ctxP is the responsibility of each module.
     * We do not clean that up here.
     */
    Tcl_Free((void *)ctxP);
}

Tclh_ReturnCode
Tclh_LibInit(Tcl_Interp *interp, Tclh_LibContext **tclhCtxPP)
{
    Tclh_LibContext *ctxP;
    const char *const ctxName = TCLH_LIB_CONTEXT_NAME;
    ctxP = (Tclh_LibContext *)Tcl_GetAssocData(interp, ctxName, NULL);

    if (ctxP == NULL) {
        ctxP = (Tclh_LibContext *)Tcl_Alloc(sizeof(*ctxP));
        memset(ctxP, 0, sizeof(*ctxP));
        ctxP->interp = interp;
        Tcl_SetAssocData(interp, ctxName, TclhCleanupLib, ctxP);

        /* Assumes Tcl stubs init already done but not tommath */
#if defined(USE_TCL_STUBS) && defined(TCLH_USE_TCL_TOMMATH)
        if (Tcl_TomMath_InitStubs(interp, 0) == NULL) {
            Tcl_Free((char *)ctxP);
            return TCL_ERROR;
        }
#endif
    }
    if (tclhCtxPP)
        *tclhCtxPP = ctxP;
    return TCL_OK;
}

void
TclhRecordErrorCode(Tcl_Interp *interp, const char *code, Tcl_Obj *msgObj)
{
    Tcl_Obj *objs[3];
    Tcl_Obj *errorCodeObj;

    TCLH_ASSERT(interp);
    TCLH_ASSERT(code);

    objs[0] = Tcl_NewStringObj(TCLH_EMBEDDER, -1);
    objs[1] = Tcl_NewStringObj(code, -1);
    objs[2] = msgObj;
    errorCodeObj = Tcl_NewListObj(msgObj == NULL ? 2 : 3, objs);
    Tcl_SetObjErrorCode(interp, errorCodeObj);
}

/*  NOTE: caller should hold a ref count to msgObj if it will be accessed
 *    when this function returns
 */
static Tclh_ReturnCode
TclhRecordError(Tcl_Interp *interp, const char *code, Tcl_Obj *msgObj)
{
    TCLH_ASSERT(code);
    TCLH_ASSERT(msgObj);
    if (interp) {
        TclhRecordErrorCode(interp, code, msgObj);
        Tcl_SetObjResult(interp, msgObj);
    }
    else {
        /*
         * If not passing to interp, we have to free msgObj. The Incr/Decr
         * dance is in case caller has a reference to it, we should not free
         * it.
         */
        Tcl_IncrRefCount(msgObj);
        Tcl_DecrRefCount(msgObj);
    }
    return TCL_ERROR;
}

Tclh_ReturnCode
Tclh_ErrorGeneric(Tcl_Interp *interp, const char *code, const char *message)
{
    Tcl_Obj *msgObj =
            Tcl_NewStringObj(message ? message : "Unknown error.", -1);
    if (code == NULL)
        code = "ERROR";
    return TclhRecordError(interp, "ERROR", msgObj);
}

Tclh_ReturnCode
Tclh_ErrorWrongType(Tcl_Interp *interp, Tcl_Obj *argObj, const char *message)
{
    Tcl_Obj *msgObj;
    if (message == NULL)
        message = "";
    if (argObj) {
        msgObj = Tcl_ObjPrintf("Value \"%s\" has the wrong type. %s",
                               Tcl_GetString(argObj), message);
    }
    else {
        msgObj = Tcl_ObjPrintf("Value has the wrong type. %s", message);
    }
    return TclhRecordError(interp, "WRONG_TYPE", msgObj);
}

Tclh_ReturnCode
Tclh_ErrorExists(Tcl_Interp *interp,
                 const char *type,
                 Tcl_Obj *searchObj,
                 const char *message)
{
    Tcl_Obj *msgObj;
    if (type == NULL)
        type = "Object";
    if (message == NULL)
        message = "";
    if (searchObj) {
        msgObj = Tcl_ObjPrintf("%s \"%s\" already exists. %s",
                               type,
                               Tcl_GetString(searchObj),
                               message);
    }
    else {
        msgObj = Tcl_ObjPrintf("%s already exists. %s", type, message);
    }
    return TclhRecordError(interp, "EXISTS", msgObj);
}

Tclh_ReturnCode
Tclh_ErrorNotFound(Tcl_Interp *interp,
                   const char *type,
                   Tcl_Obj *searchObj,
                   const char *message)
{
    return Tclh_ErrorNotFoundStr(
        interp, type, searchObj ? Tcl_GetString(searchObj) : NULL, message);
}

Tclh_ReturnCode
Tclh_ErrorNotFoundStr(Tcl_Interp *interp,
                      const char *type,
                      const char *searchStr,
                      const char *message)
{
    Tcl_Obj *msgObj;
    if (type == NULL)
        type = "Object";
    if (message == NULL)
        message = "";
    if (searchStr) {
        msgObj = Tcl_ObjPrintf("%s \"%s\" not found or inaccessible. %s",
                               type,
                               searchStr,
                               message);
    }
    else {
        msgObj = Tcl_ObjPrintf("%s not found. %s", type, message);
    }
    return TclhRecordError(interp, "NOT_FOUND", msgObj);
}

Tclh_ReturnCode
Tclh_ErrorOperFailed(Tcl_Interp *interp,
                     const char *oper,
                     Tcl_Obj *operandObj,
                     const char *message)
{
    Tcl_Obj *msgObj;
    const char *operand;
    operand = operandObj == NULL ? "object" : Tcl_GetString(operandObj);
    if (message == NULL)
        message = "";
    if (oper)
        msgObj = Tcl_ObjPrintf("Operation %s failed on %s. %s", oper, operand, message);
    else
        msgObj = Tcl_ObjPrintf("Operation failed on %s. %s", operand, message);
    return TclhRecordError(interp, "OPER_FAILED", msgObj);
}

Tclh_ReturnCode
Tclh_ErrorInvalidValueStr(Tcl_Interp *interp,
                          const char *badValue,
                          const char *message)
{
    Tcl_Obj *msgObj;
    if (message == NULL)
        message = "";
    if (badValue) {
        msgObj = Tcl_ObjPrintf("Invalid value \"%s\". %s", badValue, message);
    }
    else {
        msgObj = Tcl_ObjPrintf("Invalid value. %s", message);
    }
    return TclhRecordError(interp, "INVALID_VALUE", msgObj);
}

Tclh_ReturnCode
Tclh_ErrorInvalidValue(Tcl_Interp *interp,
                       Tcl_Obj *badArgObj,
                       const char *message)
{
    return Tclh_ErrorInvalidValueStr(
        interp, badArgObj ? Tcl_GetString(badArgObj) : NULL, message);
}

Tclh_ReturnCode
Tclh_ErrorOptionMissingStr(Tcl_Interp *interp,
                        const char *optName,
                        const char *message)
{
    Tcl_Obj *msgObj;
    if (message == NULL)
        message = "";
    if (optName) {
        msgObj = Tcl_ObjPrintf("Required option \"%s\" not specified. %s",
                               optName,
                               message);
    }
    else {
        msgObj = Tcl_ObjPrintf("Required option not specified. %s", message);
    }
    return TclhRecordError(interp, "OPTION_MISSING", msgObj);
}

Tclh_ReturnCode
Tclh_ErrorOptionValueMissing(Tcl_Interp *interp,
                             Tcl_Obj *optionNameObj,
                             const char *message)
{
    Tcl_Obj *msgObj;
    if (message == NULL)
        message = "";
    if (optionNameObj) {
        msgObj = Tcl_ObjPrintf("No value specified for option \"%s\". %s",
                               Tcl_GetString(optionNameObj),
                               message);
    }
    else {
        msgObj = Tcl_ObjPrintf("No value specified for option. %s", message);
    }
    return TclhRecordError(interp, "OPTION_VALUE_MISSING", msgObj);
}


Tclh_ReturnCode
Tclh_ErrorNumArgs(Tcl_Interp *interp,
                  int objc,
                  Tcl_Obj *const objv[],
                  const char *message)
{
    Tcl_WrongNumArgs(interp, objc, objv, message);
    return TCL_ERROR;
}

Tclh_ReturnCode
Tclh_ErrorAllocation(Tcl_Interp *interp, const char *type, const char *message)
{
    Tcl_Obj *msgObj;
    if (message == NULL)
        message = "";
    if (type == NULL)
        type = "Object";
    msgObj = Tcl_ObjPrintf("%s allocation failed. %s", type, message);
    return TclhRecordError(interp, "ALLOCATION", msgObj);
}

Tclh_ReturnCode
Tclh_ErrorRange(Tcl_Interp *interp,
                Tcl_Obj *objP,
                Tcl_WideInt low,
                Tcl_WideInt high)
{
    Tcl_Obj *msgObj;
    /* Resort to snprintf because ObjPrintf does not handle %lld correctly */
    char buf[200];
    snprintf(buf,
             sizeof(buf),
             "Value%s%.20s not in range. Must be within [%" TCL_LL_MODIFIER
             "d,%" TCL_LL_MODIFIER "d].",
             objP ? " " : "",
             objP ? Tcl_GetString(objP) : "",
             low,
             high);
    msgObj = Tcl_NewStringObj(buf, -1);
    return TclhRecordError(interp, "RANGE", msgObj);
}

/*
 * Print an address is cross-platform manner. Internal routine, minimal error checks
 */
char *TclhPrintAddress(const void *address, char *buf, int buflen)
{
    TCLH_ASSERT(buflen > 2);

    /*
     * Note we do not sue %p here because generated output differs
     * between compilers in terms of the 0x prefix. Moreover, gcc
     * prints (nil) for NULL pointers which is not what we want.
     */
    if (sizeof(void*) == sizeof(int)) {
        unsigned int i = (unsigned int) (intptr_t) address;
        snprintf(buf, buflen, "0x%.8x", i);
    }
    else {
        TCLH_ASSERT(sizeof(void *) == sizeof(unsigned long long));
        unsigned long long ull = (intptr_t)address;
        snprintf(buf, buflen, "0x%.16llx", ull);
    }
    return buf;
}

Tclh_ReturnCode
Tclh_ErrorEncodingFromUtf8(Tcl_Interp *ip,
                           int encoding_status,
                           const char *utf8,
                           Tcl_Size utf8Len)
{
    const char *message;
    char limited[80];
    if (utf8Len < 0)
        utf8Len = Tclh_strlen(utf8);
    switch (encoding_status) {
    case TCL_CONVERT_NOSPACE:
        message =
            "String length is greater than specified maximum buffer size.";
        break;
    case TCL_CONVERT_MULTIBYTE:
        message = "String ends in a partial multibyte encoding fragment.";
        break;
    case TCL_CONVERT_SYNTAX:
        message = "String contains invalid character sequence";
        break;
    case TCL_CONVERT_UNKNOWN:
        message = "String cannot be encoded in target encoding.";
        break;
    default:
        message = NULL;
        break;
    }
    /* Remember fromLen does not include nul */
    snprintf(limited,
              utf8Len < sizeof(limited) ? utf8Len : sizeof(limited),
              "%s",
              utf8);
    return Tclh_ErrorInvalidValueStr(ip, utf8, message);
}



#ifdef _WIN32
Tcl_Obj *TclhMapWindowsError(
    DWORD winError,      /* Windows error code */
    HANDLE moduleHandle,        /* Handle to module containing error string.
                                 * If NULL, assumed to be system message. */
    const char *msgPtr)         /* Message prefix. May be NULL. */
{
    Tcl_Size length;
    DWORD flags;
    WCHAR *winErrorMessagePtr = NULL;
    Tcl_Obj *objPtr;
    Tcl_DString ds;

    Tcl_DStringInit(&ds);

    if (msgPtr) {
        const char *p;
        Tcl_DStringAppend(&ds, msgPtr, -1);
        p      = Tcl_DStringValue(&ds);
        length = Tcl_DStringLength(&ds);/* Does NOT include terminating nul */
        if (length && p[length] == ' ') {
            Tcl_DStringAppend(&ds, " ", 1);
        }
    }

    flags = moduleHandle ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM;
    flags |=
        FORMAT_MESSAGE_ALLOCATE_BUFFER /* So we do not worry about length */
        | FORMAT_MESSAGE_IGNORE_INSERTS /* Ignore message arguments */
        | FORMAT_MESSAGE_MAX_WIDTH_MASK;/* Ignore soft line breaks */

    length = FormatMessageW(flags, moduleHandle, winError,
                            0, /* Lang id */
                            (WCHAR *) &winErrorMessagePtr,
                            0, NULL);
    if (length > 0) {
        /* Strip trailing CR LF if any */
        if (winErrorMessagePtr[length-1] == L'\n')
            --length;
        if (length > 0) {
            if (winErrorMessagePtr[length-1] == L'\r')
                --length;
        }
#if TCLH_TCLAPI_VERSION < 0x0807
        objPtr =
            Tcl_NewStringObj(Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
        Tcl_DStringFree(&ds);
        Tcl_AppendUnicodeToObj(objPtr, winErrorMessagePtr, length);
#else
        Tcl_WCharToUtfDString(winErrorMessagePtr, length, &ds);
        objPtr = Tcl_DStringToObj(&ds);
#endif
        LocalFree(winErrorMessagePtr);
    } else {
        objPtr =
            Tcl_NewStringObj(Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
        Tcl_DStringFree(&ds);
        Tcl_AppendPrintfToObj(objPtr, "Windows error code %ld", winError);
    }
    return objPtr;
}

Tclh_ReturnCode
Tclh_ErrorWindowsError(Tcl_Interp *interp,
                       unsigned int winerror,
                       const char *message)
{
    Tcl_Obj *msgObj = TclhMapWindowsError(winerror, NULL, message);
    return TclhRecordError(interp, "WINERROR", msgObj);
}
#endif /* _WIN32 */
