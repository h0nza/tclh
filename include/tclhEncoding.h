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
 * interp - Tcl interpreter in which to initialize.
 *
 * Returns:
 * TCL_OK    - Library was successfully initialized.
 * TCL_ERROR - Initialization failed. Library functions must not be called.
 *             An error message is left in the interpreter result.
 */
TCLH_INLINE int Tclh_EncodingLibInit(Tcl_Interp *interp) {
    return Tclh_BaseLibInit(interp);
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


#ifdef TCLH_SHORTNAMES
#define ExternalToUtf Tclh_ExternalToUtf
#define UtfToExternal Tclh_UtfToExternal
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
	    srcChunkLen = srcLen;
	    flags |= (origFlags & TCL_ENCODING_END); /* Last chunk if caller said so */
	}
	dstChunkCapacity = dstCapacity > INT_MAX ? INT_MAX : dstCapacity;

	result = Tcl_ExternalToUtf(interp, encoding, src + srcRead,
		srcChunkLen, flags, &state, dst + dstWrote, dstChunkCapacity,
		&srcChunkRead, &dstChunkWrote, &dstChunkChars);

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
        } else {
	    srcChunkLen = srcLen;
	    flags |= (origFlags & TCL_ENCODING_END); /* Last chunk if caller said so */
	}
	dstChunkCapacity = dstCapacity > INT_MAX ? INT_MAX : dstCapacity;

	result = Tcl_UtfToExternal(interp, encoding, src + srcRead,
		srcChunkLen, flags, &state, dst + dstWrote, dstChunkCapacity,
		&srcChunkRead, &dstChunkWrote, &dstChunkChars);

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

#endif /* TCLH_IMPL */
#endif /* TCLHENCODING_H */