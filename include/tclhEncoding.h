#ifndef TCLHENCODING_H
#define TCLHENCODING_H

#include "tclhBase.h"

/* Section: Tcl encoding convenience functions
 *
 * Provides some wrappers around Tcl encoding routines.
 */

/* Function: Tclh_EncodingLibInit
 * Must be called to initialize the Encoding module before any of
 * the other functions in the module.
 *
 * Parameters:
 * interp - Tcl interpreter for error messages. May be NULL.
 * tclhCtxP - Tclh context as returned by <Tclh_LibInit> to use. If NULL,
 *    the Tclh context associated with the interpreter is used after
 *    initialization if necessary.
 *
 * At least one of interp and tclhCtxP must be non-NULL.
 *
 * Returns:
 * TCL_OK    - Library was successfully initialized.
 * TCL_ERROR - Initialization failed. Library functions must not be called.
 *             An error message is left in the interpreter result.
 */
TCLH_INLINE int
Tclh_EncodingLibInit(Tcl_Interp *interp, Tclh_LibContext *tclhCtxP)
{
    if (tclhCtxP == NULL) {
        return Tclh_LibInit(interp, NULL);
    }
    return TCL_OK; /* Must have been already initialized */
}

/* Function: Tclh_ExternalToUtf
 * Wrapper around Tcl_ExternalToUtf to allow lengths > INT_MAX.
 *
 * See documentation of Tcl's Tcl_ExternalToUtf for parameters and
 * return values. The only difference is the types of *srcReadPtr*,
 * *dstWrotePtr* and *dstCharsPtr* which are all of type Tcl_Size*.
 */
#if TCL_MAJOR_VERSION >= 9
int Tclh_ExternalToUtf(Tcl_Interp *interp, Tcl_Encoding encoding,
                       const char *src, Tcl_Size srcLen, int flags,
                       Tcl_EncodingState *statePtr, char *dst,
                       Tcl_Size dstLen, Tcl_Size *srcReadPtr,
                       Tcl_Size *dstWrotePtr, Tcl_Size *dstCharsPtr);
#else
/* Directly call as sizeof(Tcl_Size) == sizeof(int) */
TCLH_INLINE int Tclh_ExternalToUtf(Tcl_Interp *interp, Tcl_Encoding encoding,
                       const char *src, Tcl_Size srcLen, int flags,
                       Tcl_EncodingState *statePtr, char *dst,
                       Tcl_Size dstLen, Tcl_Size *srcReadPtr,
                       Tcl_Size *dstWrotePtr, Tcl_Size *dstCharsPtr) {
    return Tclh_ExternalToUtf(Tcl_Interp * interp,
                              Tcl_Encoding encoding, const char *src,
                              Tcl_Size srcLen, int flags,
                              Tcl_EncodingState *statePtr, char *dst,
                              Tcl_Size dstLen, int *srcReadPtr,
                              int *dstWrotePtr, int *dstCharsPtr);
}
#endif

/* Function: Tclh_UtfToExternal
 * Wrapper around Tcl_UtfToExternal to allow lengths > INT_MAX.
 *
 * See documentation of Tcl's Tcl_UtfToExternal for parameters and
 * return values. The only difference is the types of *srcReadPtr*,
 * *dstWrotePtr* and *dstCharsPtr* which are all of type Tcl_Size*.
 */
#if TCL_MAJOR_VERSION >= 9
int Tclh_UtfToExternal(Tcl_Interp *interp, Tcl_Encoding encoding,
                       const char *src, Tcl_Size srcLen, int flags,
                       Tcl_EncodingState *statePtr, char *dst,
                       Tcl_Size dstLen, Tcl_Size *srcReadPtr,
                       Tcl_Size *dstWrotePtr, Tcl_Size *dstCharsPtr);
#else
/* Directly call as sizeof(Tcl_Size) == sizeof(int) */
TCLH_INLINE int Tclh_UtfToExternal(Tcl_Interp *interp, Tcl_Encoding encoding,
                       const char *src, Tcl_Size srcLen, int flags,
                       Tcl_EncodingState *statePtr, char *dst,
                       Tcl_Size dstLen, Tcl_Size *srcReadPtr,
                       Tcl_Size *dstWrotePtr, Tcl_Size *dstCharsPtr) {
    return Tclh_UtfToExternal(Tcl_Interp * interp,
                              Tcl_Encoding encoding, const char *src,
                              Tcl_Size srcLen, int flags,
                              Tcl_EncodingState *statePtr, char *dst,
                              Tcl_Size dstLen, int *srcReadPtr,
                              int *dstWrotePtr, int *dstCharsPtr);
}
#endif

/* Function: Tclh_ExternalToUtfAlloc
 * Transforms the input in the given encoding to Tcl's internal UTF-8
 *
 * Parameters:
 * bufPP - location to store pointer to the allocated buffer. This must be
 *    freed by calling Tcl_Free.
 * numBytesOutP - location to store number of bytes copied to the buffer
 *    not counting the terminating nul. May be NULL.
 * 
 * The other parameters are as for Tcl_ExternalToUtfDStringEx. See the Tcl
 * documentation for details. This function differs in that it returns the
 * output in an allocated buffer as opposed to the Tcl_DString structure.
 */
int Tclh_ExternalToUtfAlloc(Tcl_Interp *interp,
                              Tcl_Encoding encoding,
                              const char *src,
                              Tcl_Size srcLen,
                              int flags,
                              char **bufPP,
                              Tcl_Size *numBytesOutP,
                              Tcl_Size *errorLocPtr);

/* Function: Tclh_UtfToExternalAlloc
 * Transforms Tcl's internal UTF-8 encoded data to the given encoding
 *
 * Parameters:
 * bufPP - location to store pointer to the allocated buffer. This must be
 *    freed by calling Tcl_Free.
 * numBytesOutP - location to store number of bytes copied to the buffer
 *    not counting the terminating nul bytes. May be NULL.
 * 
 * The other parameters are as for Tcl_UtfToExternalDStringEx. See the Tcl
 * documentation for details. This function differs in that it returns the
 * output in an allocated buffer as opposed to the Tcl_DString structure.
 */
int Tclh_UtfToExternalAlloc(Tcl_Interp *interp,
                              Tcl_Encoding encoding,
                              const char *src,
                              Tcl_Size srcLen,
                              int flags,
                              char **bufPP,
                              Tcl_Size *numBytesOutP,
                              Tcl_Size *errorLocPtr);

#ifdef _WIN32
/* Function: Tclh_ObjFromWinChars
 * Returns a Tcl_Obj containing a copy of the passed WCHAR string.
 * 
 * Parameters:
 * tclhCtxP - Tclh context. May be NULL in which case a temporary Tcl_Encoding
 *    context is used.
 * wsP - pointer to a WCHAR string
 * numChars - length of the string. If negative, the string must be nul terminated.
 *
 * If the Tcl version supports encoding profiles, the encoding is converted
 * using the replace profile.
 * 
 * Returns:
 * A non-NULL Tcl_Obj. Panics of failure (memory allocation).
 */
Tcl_Obj *
Tclh_ObjFromWinChars(Tclh_LibContext *tclhCtxP, WCHAR *wsP, Tcl_Size numChars);

#endif

#ifdef TCLH_SHORTNAMES
#define ExternalToUtf Tclh_ExternalToUtf
#define UtfToExternal Tclh_UtfToExternal
#define ExternalToUtfAlloc Tclh_ExternalToUtfAlloc
#define ObjFromWinChars Tclh_ObjFromWinChars
#endif

/*
 * Begin implementation
 */

#ifdef TCLH_IMPL

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

#endif

#endif /* TCLH_IMPL */
#endif /* TCLHENCODING_H */