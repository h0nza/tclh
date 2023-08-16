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
    Tcl_Size dstLen, srcLen, dstSpace;
    Tcl_Size srcLatestRead, dstLatestWritten;
    const char *srcP;
    char *dstP;
    int status;
    Tcl_EncodingState state;

    srcP = fromP;               /* Keep fromP unchanged for error messages */
    srcLen = fromLen;

    flags = TCL_ENCODING_START | TCL_ENCODING_END;

    dstSpace = srcLen + 4; /* Possibly four nuls (UTF-32) */
    dstP = Tclh_LifoAlloc(memlifoP, dstSpace);
    dstLen = 0;
    while (1) {
        /* dstP is buffer. dstLen is what's written so far */
        status = Tclh_UtfToExternal(ip,
                                    encoding,
                                    srcP,
                                    srcLen,
                                    flags,
                                    &state,
                                    dstP + dstLen,
                                    dstSpace - dstLen,
                                    &srcLatestRead,
                                    &dstLatestWritten,
                                    NULL);
        TCLH_ASSERT(dstSpace >= dstLatestWritten);
        dstLen += dstLatestWritten;
        /* Terminate loop on any status other than space  */
        if (status != TCL_CONVERT_NOSPACE) {
            if (status == TCL_ERROR) {
                *outPP = NULL;
                if (numBytesOutP)
                    *numBytesOutP = 0;
                if (errorLocPtr) {
                    /* Do not take into account current srcLatestRead */
                    *errorLocPtr = (Tcl_Size) (srcP - fromP);
                }
            } else {
                /* Tack on 4 nuls as we don't know how many nuls encoding uses */
                if ((dstSpace - dstLen) < 4)
                    dstP = Tclh_LifoResizeLast(memlifoP, 4 + dstLen, 0);
                int i;
                for (i = 0; i < 4; ++i)
                    dstP[dstLen + i] = 0;
                if (numBytesOutP)
                    *numBytesOutP = dstLen; /* Does not include terminating nul */
                if (errorLocPtr) {
                    if (status == TCL_OK)
                        *errorLocPtr = -1;
                    else
                        *errorLocPtr =
                            (Tcl_Size)((srcP - fromP) + srcLatestRead);
                }
                *outPP = dstP;
            }
            return status;
        }
        flags &= ~ TCL_ENCODING_START;

        TCLH_ASSERT(srcLatestRead <= srcLen);
        srcP += srcLatestRead;
        srcLen -= srcLatestRead;
        Tcl_Size delta = dstSpace / 2;
        if ((TCL_SIZE_MAX-delta) < dstSpace)
            dstSpace = TCL_SIZE_MAX;
        else
            dstSpace += delta;
        dstP = Tclh_LifoResizeLast(memlifoP, dstSpace, 0);
    }
}
#endif /* TCLH_LIFO_E_SUCCESS */

#ifdef _WIN32

#if TCL_UTF_MAX >= 4

static TclhCleanupEncodings(ClientData clientData, Tcl_Interp *interp)
{
    Tcl_Encoding encoding = (Tcl_Encoding)clientData;
    if (encoding)
        Tcl_FreeEncoding(encoding);
}

Tcl_Encoding TclhGetUtf16Encoding(Tclh_LibContext *tclhCtxP)
{
    Tcl_Encoding enc;

    if (tclhCtxP == NULL || tclhCtxP->encUTF16LE == NULL) {
        enc = Tcl_GetEncoding(NULL, "utf-16le");
        if (tclhCtxP) {
            tclhCtxP->encUTF16LE = enc;
            Tcl_CallWhenDeleted(
                tclhCtxP->interp, TclhCleanupEncodings, tclhCtxP->encUTF16LE);
        }
    } else {
        enc = tclhCtxP->encUTF16LE;
    }
    TCLH_ASSERT(enc);
    return enc;
}
#endif

Tcl_Obj *
Tclh_ObjFromWinChars(Tclh_LibContext *tclhCtxP, WCHAR *wsP, Tcl_Size numChars)
{
#if TCL_UTF_MAX < 4
    return Tcl_NewUnicodeObj(wsP, numChars);
#else
    Tcl_Encoding enc;

    enc = TclhGetUtf16Encoding(tclhCtxP);
    TCLH_ASSERT(enc);

    /* 
     * Note we do not use Tcl_Char16ToUtfDString because of its shortcomings
     * with respect to encoding errors and overallocation of memory.
     */
    Tcl_DString ds;
    Tclh_ReturnCode ret;
    ret = Tcl_ExternalToUtfDStringEx(NULL,
                                     enc,
                                     (char *)wsP,
                                     numChars * sizeof(WCHAR),
                                     TCL_ENCODING_PROFILE_REPLACE,
                                     &ds,
                                     NULL);
    TCLH_ASSERT(ret == TCL_OK); /* Should never fail for REPLACE profile */
    
    /* If we don't have a tclhCtxP, we need to release the encoding */
    if (tclhCtxP == NULL) {
        Tcl_FreeEncoding(enc);
    }
    return Tcl_DStringToObj(&ds);
#endif
}
#endif /* _WIN32 */
