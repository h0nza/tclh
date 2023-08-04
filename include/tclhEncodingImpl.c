/*
 * Copyright (c) 2021-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhEncoding.h"
#include <assert.h>

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
    Tcl_EncodingState state = *statePtr;
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
    Tcl_EncodingState state = *statePtr;
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
#else
    ret = Tcl_ExternalToUtfDString(encoding, src, srcLen, &ds);
    if (errorLocPtr)
        *errorLocPtr = -1; /* Older API cannot have encoding errors */
#endif
    if (ret == TCL_ERROR)
        return TCL_ERROR;

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
#else
    ret = Tcl_UtfToExternalDString(encoding, src, srcLen, &ds);
    if (errorLocPtr)
        *errorLocPtr = -1; /* Older API cannot have encoding errors */
#endif
    if (ret == TCL_ERROR)
        return TCL_ERROR;

    /*
     * Being a bad citizen here, poking into DString internals but
     * the current public Tcl API leaves us no choice.
     */
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

#ifdef _WIN32
Tcl_Obj *
Tclh_ObjFromWinChars(Tclh_LibContext *tclhCtxP, WCHAR *wsP, Tcl_Size numChars)
{
#if TCL_UTF_MAX < 4
    return Tcl_NewUnicodeObj(wsP, numChars);
#else
    Tcl_Encoding enc;

    if (tclhCtxP == NULL || tclhCtxP->encUTF16LE == NULL) {
        enc = Tcl_GetEncoding(NULL, "utf-16le");
        if (tclhCtxP)
            tclhCtxP->encUTF16LE = enc;
    } else {
        enc = tclhCtxP->encUTF16LE;
    }
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
    return Tcl_DStringToObj(&ds);
#endif
}
#endif /* _WIN32 */