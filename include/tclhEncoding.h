/*
 * Copyright (c) 2021-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

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
    return Tcl_ExternalToUtf(interp, encoding, src, srcLen, flags, statePtr,
                             dst, dstLen, srcReadPtr, dstWrotePtr, dstCharsPtr);
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
    return Tcl_UtfToExternal(interp, encoding, src, srcLen, flags, statePtr,
                              dst, dstLen, srcReadPtr, dstWrotePtr, dstCharsPtr);
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

#ifdef TCLH_LIFO_E_SUCCESS /* Only define if Lifo module is available */

/* Function: Tclh_UtfToExternalLifo
 * Transforms Tcl's internal UTF-8 encoded data to the given encoding
 *
 * Parameters:
 * memlifoP - The Tclh_MemLifo from which to allocate memory.
 * numBytesOutP - location to store number of bytes copied to the buffer
 *    not counting the terminating nul bytes. May be NULL.
 * 
 * The other parameters are as for Tcl_UtfToExternalDStringEx. See the Tcl
 * documentation for details. This function differs in that it returns the
 * output in memory allocated from a Tclh_Lifo.
 * 
 * The *tclhLifo.h* file must be included before *tclhEncoding.h* 
 * for this function to be present.
 */
int Tclh_UtfToExternalLifo(Tcl_Interp *ip,
                           Tcl_Encoding encoding,
                           const char *fromP,
                           Tcl_Size fromLen,
                           int flags,
                           Tclh_Lifo *memlifoP,
                           char **outPP,
                           Tcl_Size *numBytesOutP,
                           Tcl_Size *errorLocPtr);

#endif

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

#ifdef TCLH_IMPL
#include "tclhEncodingImpl.c"
#endif

#endif /* TCLHENCODING_H */