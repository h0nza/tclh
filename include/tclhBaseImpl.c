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
    Tclh_Free(ctxP);
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
            Tclh_Free(ctxP);
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
    const char *sep;
    if (message == NULL)
        sep = message = "";
    else
        sep = " ";
    if (searchObj) {
        msgObj = Tcl_ObjPrintf("%s \"%s\" already exists.%s%s",
                               type,
                               Tcl_GetString(searchObj),
                               sep,
                               message);
    }
    else {
        msgObj = Tcl_ObjPrintf("%s already exists.%s%s", type, sep, message);
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
    const char *sep;
    if (message == NULL)
        sep = message = "";
    else
        sep = " ";
    if (searchStr) {
        msgObj = Tcl_ObjPrintf("%s \"%s\" not found or inaccessible.%s%s",
                               type,
                               searchStr,
                               sep,
                               message);
    }
    else {
        msgObj = Tcl_ObjPrintf("%s not found.%s%s", type, sep, message);
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
    const char *sep;
    if (message == NULL)
        sep = message = "";
    else
        sep = " ";
    if (oper)
        msgObj = Tcl_ObjPrintf("Operation %s failed on %s.%s%s", oper, operand, sep, message);
    else
        msgObj = Tcl_ObjPrintf("Operation failed on %s.%s%s", operand, sep, message);
    return TclhRecordError(interp, "OPER_FAILED", msgObj);
}

Tclh_ReturnCode
Tclh_ErrorInvalidValueStr(Tcl_Interp *interp,
                          const char *badValue,
                          const char *message)
{
    Tcl_Obj *msgObj;
    const char *sep;
    if (message == NULL)
        sep = message = "";
    else
        sep = " ";
    if (badValue) {
        msgObj = Tcl_ObjPrintf("Invalid value \"%s\".%s%s", badValue, sep, message);
    }
    else {
        msgObj = Tcl_ObjPrintf("Invalid value.%s%s", sep, message);
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
    const char *sep;
    if (message == NULL)
        sep = message = "";
    else
        sep = " ";
    if (optName) {
        msgObj = Tcl_ObjPrintf("Required option \"%s\" not specified.%s%s",
                               optName,
                               sep,
                               message);
    }
    else {
        msgObj =
            Tcl_ObjPrintf("Required option not specified.%s%s", sep, message);
    }
    return TclhRecordError(interp, "OPTION_MISSING", msgObj);
}

Tclh_ReturnCode
Tclh_ErrorOptionValueMissing(Tcl_Interp *interp,
                             Tcl_Obj *optionNameObj,
                             const char *message)
{
    Tcl_Obj *msgObj;
    const char *sep;
    if (message == NULL)
        sep = message = "";
    else
        sep = " ";
    if (optionNameObj) {
        msgObj = Tcl_ObjPrintf("No value specified for option \"%s\".%s%s",
                               Tcl_GetString(optionNameObj),
                               sep,
                               message);
    }
    else {
        msgObj = Tcl_ObjPrintf("No value specified for option.%s%s", sep, message);
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
    if (type == NULL)
        type = "Object";
    const char *sep;
    if (message == NULL)
        sep = message = "";
    else
        sep = " ";
    msgObj = Tcl_ObjPrintf("%s allocation failed.%s%s", type, sep, message);
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

    char limited[80];
    if (utf8 == NULL) {
        utf8 = "";
        utf8Len = 0;
    }
    else if (utf8Len < 0)
        utf8Len = Tclh_strlen(utf8);

    /* TODO - print utf8 but what if it is invalid encoding! May be print in hex? */
    snprintf(limited,
              sizeof(limited),
              "%s",
              utf8);

    return Tclh_ErrorInvalidValueStr(ip, limited, message);
}

/*
 * errnoname.c is from https://github.com/mentalisttraceur/errnoname.
 * See the file for license (BSD Zero)
 */
#include "errnoname.c"

Tclh_ReturnCode
Tclh_ErrorErrnoError(Tcl_Interp *interp,
                     int err,
                     const char *message)
{
    if (interp) {
        Tcl_Obj *objs[5];
        char buf[256];
        char *bufP;
        const char *errnoSym;

        objs[0] = Tcl_NewStringObj("CFFI", 4);
        objs[1] = Tcl_NewStringObj("ERRNO", 5);
        errnoSym = errnoname(err);
        if (errnoSym)
            objs[2] = Tcl_NewStringObj(errnoSym, -1);
        else
            objs[2] = Tcl_ObjPrintf("%u", err);
        objs[3] = Tcl_NewIntObj(err);
        objs[4] = Tcl_NewStringObj(message ? message : "", -1);

        buf[0] = 0;             /* Safety check against misconfig */

#ifdef _WIN32
        strerror_s(buf, sizeof(buf) / sizeof(buf[0]), (int) err);
        bufP = buf;
#else
        /*
         * This is tricky. See manpage for gcc/linux for the strerror_r
         * to be selected. BUT Apple for example does not follow the same
         * feature test macro conventions.
         */
#if _GNU_SOURCE || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE < 200112L)
        bufP = strerror_r((int) err, buf, sizeof(buf) / sizeof(buf[0]));
        /* Returned bufP may or may not be buf! */
#else
        /* XSI/POSIX standard version */
        strerror_r((int) err, buf, sizeof(buf) / sizeof(buf[0]));
        bufP = buf;
#endif
#endif

        if (message)
            Tcl_AppendToObj(objs[4], " ", 1);
        Tcl_AppendToObj(objs[4], bufP, -1);
        Tcl_SetObjErrorCode(interp, Tcl_NewListObj(5, objs));
        Tcl_SetObjResult(interp, objs[4]);
    }
    return TCL_ERROR;
}


#ifdef _WIN32

#ifdef TCLH_MAP_WINERROR_SYM
# include "tclhWinErrorSyms.c"
#endif

Tcl_Obj *TclhMapWindowsError(
    DWORD winError,      /* Windows error code */
    HANDLE moduleHandle,        /* Handle to module containing error string.
                                 * If NULL, assumed to be system message. */
    const char *msgPtr)         /* Message prefix. May be NULL. */
{
    Tcl_Size length;
    DWORD flags;
    WCHAR *winErrorMessagePtr = NULL;
    Tcl_Obj *objPtr = NULL;
    Tcl_DString ds;

    Tcl_DStringInit(&ds);

    if (msgPtr) {
        const char *p;
        Tcl_DStringAppend(&ds, msgPtr, -1);
        p      = Tcl_DStringValue(&ds);
        length = Tcl_DStringLength(&ds);/* Does NOT include terminating nul */
        if (length && p[length-1] != ' ') {
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
    while (length > 0) {
        /* Strip trailing whitespace */
        WCHAR wch = winErrorMessagePtr[length - 1];
        if (wch == L'\n' || wch == L'\r' || wch == L' ' || wch == L'\t')
            --length;
        else
            break;
    }
    if (length > 0) {
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
    } else if (moduleHandle == NULL && (winError & 0xF0000000) == 0xD0000000) {
        /* HRESULT wrapping NTSTATUS */
        HANDLE ntdllH = LoadLibrary("NTDLL.DLL");
        if (ntdllH) {
            /* Turn off 0x10000000 to map HRESULT to NTSTATUS and recurse */
            objPtr =
                TclhMapWindowsError(winError & ~0x10000000, ntdllH, msgPtr);
            FreeLibrary(ntdllH);
        }
    }
    if (objPtr == NULL) {
        objPtr =
            Tcl_NewStringObj(Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
        Tcl_DStringFree(&ds);
        Tcl_AppendPrintfToObj(objPtr, "Error code %ld", winError);
    }
    return objPtr;
}

Tclh_ReturnCode
Tclh_ErrorWindowsError(Tcl_Interp *interp,
                       unsigned int winerror,
                       const char *message)
{
    if (interp) {
        Tcl_Obj *objs[4];
        objs[0] = Tcl_NewStringObj("CFFI", 4);
        objs[1] = Tcl_NewStringObj("WIN32", 5);
#ifdef TCLH_MAP_WINERROR_SYM
        const char *sym = winerrsym(winerror);
        objs[2] =
            sym ? Tcl_NewStringObj(sym, -1) : Tcl_ObjPrintf("%u", winerror);
#else
        objs[2] = Tcl_ObjPrintf("%u", winerror);
#endif
        objs[3] = TclhMapWindowsError(winerror, NULL, message);
        Tcl_SetObjErrorCode(interp, Tcl_NewListObj(4, objs));
        Tcl_SetObjResult(interp, objs[3]);
    }
    return TCL_ERROR;
}
#endif /* _WIN32 */
