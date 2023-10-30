/*
 * Copyright (c) 2021-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhEncoding.h"
#include <assert.h>

Tclh_ReturnCode
Tclh_EncodingLibInit(Tcl_Interp *interp, Tclh_LibContext *tclhCtxP)
{
    if (tclhCtxP == NULL) {
        return Tclh_LibInit(interp, NULL);
    }
    return TCL_OK; /* Must have been already initialized */
}

#ifndef TCLH_TCL87API
Tcl_Size Tclh_GetEncodingNulLength (Tcl_Encoding encoding)
{
    if (encoding) {
        const char *encName = Tcl_GetEncodingName(encoding);
        if (encName) {
            if (!strcmp(encName, "unicode"))
                return 2;
            if (!strcmp(encName, "ascii") || !strcmp(encName, "utf-8")
                || !strncmp(encName, "iso8859-", 8)) {
                return 1;
            }
        }
    }

    /* The long way */
    char buf[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    int status;
    status = Tcl_UtfToExternal(NULL,
                               encoding,
                               "",
                               0,
                               TCL_ENCODING_START | TCL_ENCODING_END,
                               NULL,
                               buf,
                               sizeof(buf),
                               NULL,
                               NULL,
                               NULL);
    TCLH_ASSERT(status == TCL_OK);
    int i;
    for (i = 0; i < sizeof(buf); ++i) {
        if (buf[i])
            break;
    }
    return i;
}
#endif

#if TCL_MAJOR_VERSION >= 9
int
Tclh_ExternalToUtf(Tcl_Interp *interp,
                   Tcl_Encoding encoding,
                   const char * const src,
                   Tcl_Size srcLen,
                   int flags,
                   Tcl_EncodingState *statePtr,
                   char * const dst,
                   Tcl_Size dstCapacity,
                   Tcl_Size *srcReadPtr,
                   Tcl_Size *dstWrotePtr,
                   Tcl_Size *dstCharsPtr)
{
    int origFlags     = flags;
    Tcl_Size srcRead  = 0;
    Tcl_Size dstWrote = 0;
    Tcl_Size dstChars = 0;
    Tcl_EncodingState state;

    if (statePtr)
        state = *statePtr;
    else
        statePtr = &state;

    /*
     * Loop passing the wrapped function less than INT_MAX bytes each iteration.
     */
    while (1) {
        int result;
        int srcChunkLen, srcChunkRead;
        int dstChunkCapacity, dstChunkWrote, dstChunkChars;

        if (srcLen > INT_MAX) {
            srcChunkLen = INT_MAX;
            /* We are passing in only a fragment so ensure END is off */
            flags &= ~TCL_ENCODING_END;
        } else {
            srcChunkLen = (int) srcLen;
            flags |= (origFlags & TCL_ENCODING_END); /* Last chunk if caller said so */
        }
        dstChunkCapacity = dstCapacity > INT_MAX ? INT_MAX : (int) dstCapacity;

        result = Tcl_ExternalToUtf(interp,
                                   encoding,
                                   src + srcRead,
                                   srcChunkLen,
                                   flags,
                                   statePtr,
                                   dst + dstWrote,
                                   dstChunkCapacity,
                                   &srcChunkRead,
                                   &dstChunkWrote,
                                   &dstChunkChars);

        srcRead += srcChunkRead;
        dstWrote += dstChunkWrote;
        assert(dstChunkWrote <= dstChunkCapacity);
        dstCapacity -= dstChunkWrote;
        dstChars += dstChunkChars;

        /*
         * Stop the loop when either
         * - no more input
         * - TCL_CONVERT_SYNTAX, TCL_CONVERT_UNKNOWN - encoding error
         * - TCL_CONVERT_NOSPACE - no space in destination
         * 
         * TCL_OK, TCL_CONVERT_MULTIBYTE - keep going for rest of input
         */
        switch (result) {
        case TCL_OK:
        case TCL_CONVERT_MULTIBYTE:
            if (srcLen > srcRead) {
                break; /* More input, keep looping */
            }
            /* FALLTHRU - no more input */
        case TCL_CONVERT_NOSPACE:
        case TCL_CONVERT_SYNTAX:
        case TCL_CONVERT_UNKNOWN:
            /* Errors or all input processed */
            if (srcReadPtr)
                *srcReadPtr = srcRead;
            if (dstWrotePtr)
                *dstWrotePtr = dstWrote;
            if (dstCharsPtr)
                *dstCharsPtr = dstChars;
            if (statePtr)
                *statePtr = state;
            return result;
        }
    }
}
#endif

#if TCL_MAJOR_VERSION >= 9
int
Tclh_UtfToExternal(Tcl_Interp *interp,
                   Tcl_Encoding encoding,
                   const char * const src,
                   Tcl_Size srcLen,
                   int flags,
                   Tcl_EncodingState *statePtr,
                   char * const dst,
                   Tcl_Size dstCapacity,
                   Tcl_Size *srcReadPtr,
                   Tcl_Size *dstWrotePtr,
                   Tcl_Size *dstCharsPtr)
{
    int origFlags     = flags;
    Tcl_Size srcRead  = 0;
    Tcl_Size dstWrote = 0;
    Tcl_Size dstChars = 0;
    Tcl_EncodingState state;

    if (statePtr)
        state = *statePtr;
    else
        statePtr = &state;

    /*
     * Loop passing the wrapped function less than INT_MAX bytes each iteration.
     */
    while (1) {
        int result;
        int srcChunkLen, srcChunkRead;
        int dstChunkCapacity, dstChunkWrote, dstChunkChars;

        if (srcLen > INT_MAX) {
            srcChunkLen = INT_MAX;
            /* We are passing in only a fragment so ensure END is off */
            flags &= ~TCL_ENCODING_END;
        }
        else {
            srcChunkLen = (int) srcLen;
            flags |= (origFlags
                      & TCL_ENCODING_END); /* Last chunk if caller said so */
        }
        dstChunkCapacity = dstCapacity > INT_MAX ? INT_MAX : (int)dstCapacity;

        result = Tcl_UtfToExternal(interp,
                                   encoding,
                                   src + srcRead,
                                   srcChunkLen,
                                   flags,
                                   &state,
                                   dst + dstWrote,
                                   dstChunkCapacity,
                                   &srcChunkRead,
                                   &dstChunkWrote,
                                   &dstChunkChars);

        srcRead += srcChunkRead;
        dstWrote += dstChunkWrote;
        assert(dstChunkWrote <= dstChunkCapacity);
        dstCapacity -= dstChunkWrote;
        dstChars += dstChunkChars;

        /*
         * Stop the loop when either
         * - no more input
         * - TCL_CONVERT_SYNTAX, TCL_CONVERT_UNKNOWN - encoding error
         * - TCL_CONVERT_NOSPACE - no space in destination
         *
         * TCL_OK, TCL_CONVERT_MULTIBYTE - keep going for rest of input
         */
        switch (result) {
        case TCL_OK:
        case TCL_CONVERT_MULTIBYTE:
            if (srcLen > srcRead) {
                break; /* More input, keep looping */
            }
            /* FALLTHRU - no more input */
        case TCL_CONVERT_NOSPACE:
        case TCL_CONVERT_SYNTAX:
        case TCL_CONVERT_UNKNOWN:
            /* Errors or all input processed */
            if (srcReadPtr)
                *srcReadPtr = srcRead;
            if (dstWrotePtr)
                *dstWrotePtr = dstWrote;
            if (dstCharsPtr)
                *dstCharsPtr = dstChars;
            if (statePtr)
                *statePtr = state;
            return result;
        }
    }
}
#endif

int
Tclh_ExternalToUtfAlloc(
    Tcl_Interp *interp,
    Tcl_Encoding encoding,
    const char *src,
    Tcl_Size srcLen,
    int flags,
    char **bufPP,
    Tcl_Size *numBytesOutP,
    Tcl_Size *errorLocPtr)
{
    Tcl_DString ds;
    int ret;

#ifdef TCLH_TCL87API
    ret = Tcl_ExternalToUtfDStringEx(
        interp, encoding, src, srcLen, flags, &ds, errorLocPtr);
    if (ret == TCL_ERROR) {
        *bufPP = NULL;
        if (numBytesOutP)
            *numBytesOutP = 0;
        return TCL_ERROR;
    }
#else
    /* Older API cannot have encoding errors */
    Tcl_ExternalToUtfDString(encoding, src, srcLen, &ds);
    if (errorLocPtr)
        *errorLocPtr = -1;
    ret = TCL_OK;
#endif
    /* ret is one of TCL_OK or TCL_CONVERT_* codes */

    /*
     * Being a bad citizen here, poking into DString internals but
     * the current public Tcl API leaves us no choice.
     */
    if (numBytesOutP)
        *numBytesOutP = ds.length;/* Not including terminator */
    if (ds.string == ds.staticSpace) {
        *bufPP = Tcl_Alloc(ds.length + 1);
        memmove(*bufPP, ds.string, ds.length + 1);/* Includes terminating nul */
    }
    else {
        /* Move over the allocated buffer to save a copy */
        *bufPP = ds.string;
    }
    /* !!! ds fields are garbage at this point do NOT access!!!! */
    return ret;
}

int
Tclh_UtfToExternalAlloc(
    Tcl_Interp *interp,
    Tcl_Encoding encoding,
    const char *src,
    Tcl_Size srcLen,
    int flags,
    char **bufPP,
    Tcl_Size *numBytesOutP,
    Tcl_Size *errorLocPtr)
{
    Tcl_DString ds;
    int ret;

#ifdef TCLH_TCL87API
    ret = Tcl_UtfToExternalDStringEx(
        interp, encoding, src, srcLen, flags, &ds, errorLocPtr);
    if (ret == TCL_ERROR) {
        *bufPP = NULL;
        if (numBytesOutP)
            *numBytesOutP = 0;
        return TCL_ERROR;
    }
#else
    /* Older API cannot have encoding errors */
    Tcl_UtfToExternalDString(encoding, src, srcLen, &ds);
    if (errorLocPtr)
        *errorLocPtr = -1;
    ret = TCL_OK;
#endif

    /* ret is one of TCL_OK or TCL_CONVERT_* codes */

    /*
     * Being a bad citizen here, poking into DString internals but
     * the current public Tcl API leaves us no choice.
     */
    if (numBytesOutP)
        *numBytesOutP = ds.length;/* Not including terminator */

    if (numBytesOutP)
        *numBytesOutP = ds.length;/* Not including terminator */
    if (ds.string == ds.staticSpace) {
        /* We do not know terminator size so allocate the whole buffer */
        *bufPP = Tcl_Alloc(sizeof(ds.staticSpace));
        memmove(*bufPP, ds.string, sizeof(ds.staticSpace));
    }
    else {
        /* Move over the allocated buffer to save a copy */
        *bufPP = ds.string;
    }
    /* !!! ds fields are garbage at this point do NOT access!!!! */
    return ret;
}

#ifdef TCLH_LIFO_E_SUCCESS

typedef struct UtfToExternalLifoContext {
    Tcl_Encoding encoding;
    Tcl_Size nulLength;
    Tclh_Lifo *memlifoP;
    char *bufP;
    Tcl_Size bufSize;
    Tcl_Size bufUsed;
} UtfToExternalLifoContext;

static int
TclhUtfToExternalLifoHelper(Tcl_Interp *ip,
                            const char *srcP,
                            Tcl_Size srcLen,
                            int flags, /* Only for TCL_ENCODING_PROFILE_* */
                            UtfToExternalLifoContext *encCtxP,
                            char **outPP,
                            Tcl_Size *errorLocPtr
                            )
{
    Tcl_Size nulLength = encCtxP->nulLength;

    TCLH_ASSERT(encCtxP->bufSize > 0);
    TCLH_ASSERT(encCtxP->bufUsed >= 0 && encCtxP->bufUsed <= encCtxP->bufSize);

    if (srcLen < 0)
        srcLen = Tclh_strlen(srcP);

    Tcl_Size origSrcLen = srcLen;
    Tcl_Size origUsed = encCtxP->bufUsed;

    flags |= TCL_ENCODING_START | TCL_ENCODING_END;

    /*
     * Keep looping passing at most INT_MAX in each iteration.
     */
    Tcl_EncodingState state;
    while (1) {
        int status;
        Tcl_Size dstEstimate;
        Tcl_Size dstSpace = encCtxP->bufSize - encCtxP->bufUsed;

        if ((TCL_SIZE_MAX/nulLength) > srcLen)
            dstEstimate = srcLen * nulLength;
        else
            dstEstimate = srcLen;
        if ((TCL_SIZE_MAX-nulLength) >= dstEstimate)
            dstEstimate += nulLength;
        if (dstEstimate < 6)
            dstEstimate = 6; /* Quirk of Tcl encoders, at least space for one max UTF-8 sequence */
        if (dstSpace < dstEstimate) {
            void *newP = Tclh_LifoExpandLast(
                encCtxP->memlifoP, dstEstimate - dstSpace, 0);
            if (newP == NULL) {
                goto alloc_fail;
            }
            encCtxP->bufP = (char *)newP;
            encCtxP->bufSize += dstEstimate - dstSpace;
            dstSpace = dstEstimate;
        }

        /* We can only feed in max INT_MAX at a time */
        int srcChunkLen, srcChunkRead;
        int dstChunkCapacity, dstChunkWrote;
        if (srcLen > INT_MAX) {
            srcChunkLen = INT_MAX;
            /* We are passing in only a fragment so ensure END is off */
            flags &= ~TCL_ENCODING_END;
        }
        else {
            srcChunkLen = (int) srcLen;
            flags |= TCL_ENCODING_END;
        }

        dstChunkCapacity = dstSpace > INT_MAX ? INT_MAX : (int)dstSpace;

        srcChunkRead = 0;
        dstChunkWrote = 0;
        status = Tcl_UtfToExternal(ip,
                                   encCtxP->encoding,
                                   srcP,
                                   srcChunkLen,
                                   flags,
                                   &state,
                                   encCtxP->bufP + encCtxP->bufUsed,
                                   dstChunkCapacity,
                                   &srcChunkRead,
                                   &dstChunkWrote,
                                   NULL);
        TCLH_ASSERT(srcChunkRead <= srcChunkLen);
        TCLH_ASSERT(dstChunkWrote <= dstChunkCapacity);
        encCtxP->bufUsed += dstChunkWrote;
        srcP += srcChunkRead;
        srcLen -= srcChunkRead;
        switch (status) {
        case TCL_CONVERT_NOSPACE:
            break; /* Keep looping to expand destination */
        case TCL_CONVERT_MULTIBYTE:
        case TCL_OK:
            if (srcLen > 0)
                break; /* Not done yet */
            /* Fall thru to return */
        case TCL_CONVERT_SYNTAX:
        case TCL_CONVERT_UNKNOWN:
            /* Add terminating nul */
            if ((encCtxP->bufSize - encCtxP->bufUsed) < nulLength) {
                void *newP = Tclh_LifoExpandLast(
                    encCtxP->memlifoP, nulLength, 0); /* May be slight overalloc*/
                if (newP == NULL) {
                    goto alloc_fail;
                }
                encCtxP->bufP = (char *)newP;
                encCtxP->bufSize += nulLength;
            }
            memset(encCtxP->bufP + encCtxP->bufUsed, 0, nulLength);
            encCtxP->bufUsed += nulLength;
            if (outPP)
                *outPP = origUsed + encCtxP->bufP;
            if (errorLocPtr) {
                *errorLocPtr = status == TCL_OK ? -1 : origSrcLen - srcLen;
            }
            return status;

        case TCL_ERROR:
        default:
            Tclh_ErrorEncodingFromUtf8(ip, status, NULL, 0);
            /* Reset values */
            encCtxP->bufUsed = origUsed;
            if (outPP)
                *outPP = NULL;
            if (errorLocPtr)
                *errorLocPtr = origSrcLen - srcLen;
            return TCL_ERROR;
        }
        /* Keep looping, more input to be processed */
    }

alloc_fail:
    encCtxP->bufUsed = origUsed;
    Tclh_ErrorAllocation(
        ip, "buffer", "Allocation of external encoding data failed.");
    if (outPP)
        *outPP = NULL;
    return TCL_CONVERT_NOSPACE;
}

int
Tclh_UtfToExternalLifo(Tcl_Interp *ip,
                       Tcl_Encoding encoding,
                       const char *fromP,
                       Tcl_Size fromLen,
                       int flags,
                       Tclh_Lifo *memlifoP,
                       char **outPP,
                       Tcl_Size *numBytesOutP,
                       Tcl_Size *errorLocPtr)
{
    UtfToExternalLifoContext encCtx;

    encCtx.encoding = encoding;
    encCtx.nulLength = Tclh_GetEncodingNulLength(encoding);
    encCtx.memlifoP  = memlifoP;
    encCtx.bufSize   = 256;
    encCtx.bufP      = Tclh_LifoAlloc(memlifoP, encCtx.bufSize);
    encCtx.bufUsed   = 0;

    int status = TclhUtfToExternalLifoHelper(
        ip, fromP, fromLen, flags, &encCtx, outPP, errorLocPtr);
    if (status == TCL_ERROR) {
        *outPP = NULL;
        if (numBytesOutP)
            *numBytesOutP = 0;
    }
    else {
        if (numBytesOutP)
            *numBytesOutP = encCtx.bufUsed - encCtx.nulLength; /* without end nul */
    }
    return status;
}

void *
Tclh_ObjToMultiSzLifo(Tclh_LibContext *tclhCtxP,
                      Tcl_Encoding encoding,
                      Tclh_Lifo *memlifoP,
                      Tcl_Obj *objP,
                      int flags,
                      Tcl_Size *numElemsP,
                      Tcl_Size *numBytesP)
{
    UtfToExternalLifoContext encCtx;
    Tcl_Size i, numElems;
    Tcl_Interp *ip = tclhCtxP ? tclhCtxP->interp : NULL;

    if (Tcl_ListObjLength(ip, objP, &numElems) != TCL_OK)
        return NULL;

    encCtx.encoding = encoding;
    encCtx.nulLength = Tclh_GetEncodingNulLength(encoding);
    encCtx.memlifoP  = memlifoP;
    encCtx.bufSize   = 256;
    encCtx.bufP      = Tclh_LifoAlloc(memlifoP, encCtx.bufSize);
    encCtx.bufUsed   = 0;

    for (i = 0; i < numElems; ++i) {
        Tcl_Obj *elemObj;
        char *s;
        Tcl_Size len;
        int status;
        if (Tcl_ListObjIndex(ip, objP, i, &elemObj) != TCL_OK)
            return NULL;
        Tcl_IncrRefCount(elemObj);
        s = Tcl_GetStringFromObj(elemObj, &len);
        status = TclhUtfToExternalLifoHelper(
            ip, s, len, flags, &encCtx, NULL, NULL);
        if (status != TCL_OK) {
            Tclh_ErrorEncodingFromUtf8(ip, status, s, len);
            Tcl_DecrRefCount(elemObj);
            if (numElemsP)
                *numElemsP = 0;
            if (numBytesP)
                *numBytesP = 0;
            return NULL;
        }
        Tcl_DecrRefCount(elemObj);
    } /* for i < numElems */

    /* Tack on the empty string at end */
    void *newP = Tclh_LifoExpandLast(encCtx.memlifoP, encCtx.nulLength, 0);
    if (newP == NULL) {
        Tclh_ErrorAllocation(
            ip, "buffer", "Allocation of external encoding data failed.");
        if (numElemsP)
            *numElemsP = 0;
        if (numBytesP)
            *numBytesP = 0;
        return NULL;
    }
    memset(encCtx.bufUsed + encCtx.bufP, 0, encCtx.nulLength);
    encCtx.bufUsed += encCtx.nulLength;
    if (numElemsP)
        *numElemsP = numElems;
    if (numBytesP)
        *numBytesP = encCtx.bufUsed;

    return encCtx.bufP;
}

#endif /* TCLH_LIFO_E_SUCCESS */

#ifdef _WIN32

static void
TclhCleanupEncodings(ClientData clientData, Tcl_Interp *interp)
{
    Tcl_Encoding encoding = (Tcl_Encoding)clientData;
    if (encoding)
        Tcl_FreeEncoding(encoding);
}

Tcl_Encoding TclhGetUtf16Encoding(Tclh_LibContext *tclhCtxP)
{
    Tcl_Encoding enc;

    if (tclhCtxP == NULL || tclhCtxP->encWinChar == NULL) {
        enc = Tcl_GetEncoding(NULL,
#ifdef TCLH_TCL87API
                              "utf-16le"
#else
                              "unicode"
#endif
        );
        if (tclhCtxP) {
            tclhCtxP->encWinChar = enc;
            Tcl_CallWhenDeleted(
                tclhCtxP->interp, TclhCleanupEncodings, tclhCtxP->encWinChar);
        }
    } else {
        enc = tclhCtxP->encWinChar;
    }
    TCLH_ASSERT(enc);
    return enc;
}

Tcl_Obj *
Tclh_ObjFromWinChars(Tclh_LibContext *tclhCtxP, WCHAR *wsP, Tcl_Size numChars)
{
#if TCL_UTF_MAX < 4
    return Tcl_NewUnicodeObj(wsP, numChars);
#else
    Tcl_Encoding enc;

    enc = TclhGetUtf16Encoding(tclhCtxP);
    TCLH_ASSERT(enc);

    if (wsP == NULL) {
        return Tcl_NewObj(); /* Like Tcl_NewUnicodeObj */
    }
    /* 
     * Note we do not use Tcl_Char16ToUtfDString because of its shortcomings
     * with respect to encoding errors and overallocation of memory.
     */
    Tcl_DString ds;
    Tclh_ReturnCode ret;
    ret = Tcl_ExternalToUtfDStringEx(tclhCtxP->interp,
                                     enc,
                                     (char *)wsP,
                                     numChars < 0 ? -1 : numChars * sizeof(WCHAR),
                                     TCL_ENCODING_PROFILE_REPLACE,
                                     &ds,
                                     NULL);
    /* If we don't have a tclhCtxP, we need to release the encoding */
    if (tclhCtxP == NULL) {
        Tcl_FreeEncoding(enc);
    }
    if (ret != TCL_OK) {
        /* Should not really happen */
        Tcl_DStringFree(&ds);
        return NULL;
    }
    return Tcl_DStringToObj(&ds);
#endif
}

int
Tclh_UtfToWinChars(Tclh_LibContext *tclhCtxP,
                   const char *srcP,
                   Tcl_Size srcLen,
                   WCHAR *dstP,
                   Tcl_Size dstCapacity,
                   Tcl_Size *numCharsP
                   )
{
    Tcl_Encoding enc;
    int ret;

    enc = TclhGetUtf16Encoding(tclhCtxP);
    TCLH_ASSERT(enc);

    ret = Tclh_UtfToExternal(tclhCtxP->interp,
                             enc,
                             srcP,
                             srcLen,
#ifdef TCLH_TCL87API
                             TCL_ENCODING_PROFILE_REPLACE |
#endif
                                 TCL_ENCODING_START | TCL_ENCODING_END,
                             NULL,
                             (char *)dstP,
                             dstCapacity * sizeof(WCHAR),
                             NULL,
                             NULL,
                             numCharsP);
    
    /* If we don't have a tclhCtxP, we need to release the encoding */
    if (tclhCtxP == NULL) {
        Tcl_FreeEncoding(enc);
    }
    return ret;
}

#ifdef TCLH_LIFO_E_SUCCESS
WCHAR *Tclh_ObjToWinCharsLifo(Tclh_LibContext *tclhCtxP,
                              Tclh_Lifo *memLifoP,
                              Tcl_Obj *objP,
                              Tcl_Size *numCharsP)
{
    Tcl_Encoding enc;

    enc = TclhGetUtf16Encoding(tclhCtxP);
    TCLH_ASSERT(enc);

    Tcl_Size numBytes;
    Tcl_Size fromLen;
    const char *fromP;
    WCHAR *wsP;
    int ret;

    fromP = Tcl_GetStringFromObj(objP, &fromLen);
    ret   = Tclh_UtfToExternalLifo(tclhCtxP ? tclhCtxP->interp : NULL,
                                 enc,
                                 fromP,
                                 fromLen,
#ifdef TCLH_TCL87API
                                 TCL_ENCODING_PROFILE_REPLACE |
#endif
                                     TCL_ENCODING_START | TCL_ENCODING_END,
                                 memLifoP,
                                 (char **) &wsP,
                                 &numBytes,
                                 NULL);
    TCLH_ASSERT(ret == TCL_OK); /* Because REPLACE profile => no encoding errors
                                   and Lifo allocation => no memory alloc errors */
    if (ret != TCL_OK) {
        return NULL;
    }
    if (numCharsP)
        *numCharsP = numBytes / sizeof(WCHAR);
    return wsP;
}

WCHAR *
Tclh_ObjToWinCharsMultiLifo(Tclh_LibContext *tclhCtxP,
                            Tclh_Lifo *memLifoP,
                            Tcl_Obj *objP,
                            Tcl_Size *numElemsP,
                            Tcl_Size *numBytesP)
{
    Tcl_Encoding enc;

    enc = TclhGetUtf16Encoding(tclhCtxP);
    TCLH_ASSERT(enc);

    return (WCHAR *)Tclh_ObjToMultiSzLifo(tclhCtxP,
                                          enc,
                                          memLifoP,
                                          objP,
#ifdef TCLH_TCL87API
                                          TCL_ENCODING_PROFILE_REPLACE,
#else
                                          0,
#endif
                                          numElemsP,
                                          numBytesP);
}

Tcl_Obj *
Tclh_ObjFromWinCharsMulti(Tclh_LibContext *tclhCtxP,
                          WCHAR *lpcw,
                          Tcl_Size maxlen)
{
    Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
    WCHAR *start = lpcw;

    if (lpcw == NULL || maxlen == 0)
        return listObj;

    if (maxlen == -1)
        maxlen = INT_MAX;

    while ((lpcw - start) < maxlen && *lpcw) {
        WCHAR *s;
        /* Locate end of this string */
        s = lpcw;
        while ((lpcw - start) < maxlen && *lpcw)
            ++lpcw;
        if (s == lpcw) {
            /* Zero-length string - end of multi-sz */
            break;
        }
        Tcl_ListObjAppendElement(
            NULL, listObj, Tclh_ObjFromWinChars(tclhCtxP, s, (Tcl_Size) (lpcw - s)));
        ++lpcw;            /* Point beyond this string, possibly beyond end */
    }

    return listObj;
}

#endif /* TCLH_LIFO_E_SUCCESS */

#endif /* _WIN32 */
