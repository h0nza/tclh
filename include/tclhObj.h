#ifndef TCLHOBJ_H
#define TCLHOBJ_H

/*
 * Copyright (c) 2021-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhBase.h"
#include <limits.h>
#include <errno.h>
#include <ctype.h>

/* Section: Tcl_Obj convenience functions
 *
 * Provides some simple convenience functions to deal with Tcl_Obj values. These
 * may be for converting Tcl_Obj between additional native types as well as
 * for existing types like 32-bit integers where Tcl already provides functions.
 * In the latter case, the purpose is simply to reduce code bloat arising from
 * indirection through the stubs table.
 */

/* Function: Tclh_ObjLibInit
 * Must be called to initialize the Obj module before any of
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
TCLH_LOCAL Tclh_ReturnCode Tclh_ObjLibInit(Tcl_Interp *interp, Tclh_LibContext *tclhCtxP);


/* Function: Tclh_GetObjTypeDescriptor
 * Returns the type descriptor for a Tcl_Obj internal type.
 *
 * Parameters:
 * typename - name of the type
 * 
 * This a wrapper around Tcl_GetObjType which attempts to locate type
 * descriptors for unregistered types as well.
 *
 * Returns:
 * Pointer to a type descriptor or NULL if not found.
 */
TCLH_LOCAL const Tcl_ObjType *Tclh_GetObjTypeDescriptor(const char *typename);

/* Function: Tclh_ObjClearPtr
 * Releases a pointer to a *Tcl_Obj* and clears it.
 *
 * Parameters:
 * objPP - pointer to the pointer to a *Tcl_Obj*
 *
 * Decrements the reference count on the Tcl_Obj possibly freeing
 * it. The pointer itself is set to NULL.
 *
 * The pointer *objPP (but not objPP) may be NULL.
 */
TCLH_INLINE void Tclh_ObjClearPtr(Tcl_Obj **objPP) {
    if (*objPP) {
        Tcl_DecrRefCount(*objPP);
        *objPP = NULL;
    }
}

/* Function: Tclh_FreeIfNoRefs
 * Frees a Tcl_Obj if there are no references to it.
 *
 * On Tcl 9, this maps directly to the Tcl_BounceRefCount function.
 *
 * Parameters:
 * objP - pointer to Tcl_Obj.
 */
TCLH_INLINE void Tclh_FreeIfNoRefs(Tcl_Obj *objP) {
#if TCL_MAJOR_VERSION > 8
    Tcl_BounceRefCount(objP);
#else
    /* This sequence is NOT a no-op! */
    Tcl_IncrRefCount(objP);
    Tcl_DecrRefCount(objP);
#endif
}

/* Function: Tclh_ObjToRangedInt
 * Unwraps a Tcl_Obj into a Tcl_WideInt if it is within a specified range.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * low - low end of the range
 * high - high end of the range
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains a number in the specified range. Otherwise returns
 * TCL_ERROR with an error message in the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ObjToRangedInt(Tcl_Interp *interp,
                                    Tcl_Obj *obj,
                                    Tcl_WideInt low,
                                    Tcl_WideInt high,
                                    Tcl_WideInt *ptr);

/* Function: Tclh_ObjToChar
 * Unwraps a Tcl_Obj into a C *char* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *char* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_ObjToChar(Tcl_Interp *interp, Tcl_Obj *obj, signed char *ptr);

/* Function: Tclh_ObjToUChar
 * Unwraps a Tcl_Obj into a C *unsigned char* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *unsigned char* type.
 * Otherwise, returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_ObjToUChar(Tcl_Interp *interp, Tcl_Obj *obj, unsigned char *ptr);

/* Function: Tclh_ObjToShort
 * Unwraps a Tcl_Obj into a C *short* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *short* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ObjToShort(Tcl_Interp *interp, Tcl_Obj *obj, short *ptr);

/* Function: Tclh_ObjToUShort
 * Unwraps a Tcl_Obj into a C *unsigned short* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr*
 * if the passed Tcl_Obj contains an integer that fits in a C *unsigned short*
 * type. Otherwise returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_ObjToUShort(Tcl_Interp *interp, Tcl_Obj *obj, unsigned short *ptr);

/* Function: Tclh_ObjToInt
 * Unwraps a Tcl_Obj into a C *int* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *int* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ObjToInt(Tcl_Interp *interp, Tcl_Obj *obj, int *ptr);

/* Function: Tclh_ObjFromInt
 * Returns a Tcl_Obj wrapping a *int*
 *
 * Parameters:
 *  intVal - int value to be wrapped
 *
 * Returns:
 * A pointer to a Tcl_Obj with a zero reference count.
 */
TCLH_INLINE Tcl_Obj *Tclh_ObjFromInt(int intVal) {
    return Tcl_NewIntObj(intVal);
}

/* Function: Tclh_ObjToSizeInt
 * Unwraps a Tcl_Obj into a Tcl *Tcl_Size* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *Tcl_Size* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_INLINE Tclh_ReturnCode Tclh_ObjToSizeInt(Tcl_Interp *interp, Tcl_Obj *obj, Tcl_Size *ptr) {
    return Tcl_GetSizeIntFromObj(interp, obj, ptr);
}

/* Function: Tclh_ObjFromSizeInt
 * Returns a Tcl_Obj wrapping a *int*
 *
 * Parameters:
 *  intVal - int value to be wrapped
 *
 * Returns:
 * A pointer to a Tcl_Obj with a zero reference count.
 */
TCLH_INLINE Tcl_Obj *Tclh_ObjFromSizeInt(Tcl_Size val) {
    if (sizeof(int) == sizeof(Tcl_Size)) {
        return Tcl_NewIntObj(val);
    } else {
        return Tcl_NewWideIntObj(val);
    }
}

/* Function: Tclh_ObjToUInt
 * Unwraps a Tcl_Obj into a C *unsigned int* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *unsigned int* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_ObjToUInt(Tcl_Interp *interp, Tcl_Obj *obj, unsigned int *ptr);

/* Function: Tclh_ObjToLong
 * Unwraps a Tcl_Obj into a C *long* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *long* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ObjToLong(Tcl_Interp *interp,
                                          Tcl_Obj *obj,
                                          long *ptr);

/* Function: Tclh_ObjFromLong
 * Returns a Tcl_Obj wrapping a *long*
 *
 * Parameters:
 *  ll - long value to be wrapped
 *
 * Returns:
 * A pointer to a Tcl_Obj with a zero reference count.
 */
TCLH_INLINE Tcl_Obj *Tclh_ObjFromLong(long longVal) {
    return Tcl_NewLongObj(longVal);
}

/* Function: Tclh_ObjToULong
 * Unwraps a Tcl_Obj into a C *unsigned long* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *long* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_ObjToULong(Tcl_Interp *interp, Tcl_Obj *obj, unsigned long *ptr);

/* Function: Tclh_ObjFromULong
 * Returns a Tcl_Obj wrapping a *unsigned long*
 *
 * Parameters:
 *  ull - unsigned long value to be wrapped
 *
 * Returns:
 * A pointer to a Tcl_Obj with a zero reference count.
 */
TCLH_LOCAL Tcl_Obj *Tclh_ObjFromULong(unsigned long ull);

/* Function: Tclh_ObjToWideInt
 * Unwraps a Tcl_Obj into a C *Tcl_WideInt* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *Tcl_WideInt* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
#ifndef TCLH_TCL87API
TCLH_LOCAL Tclh_ReturnCode
Tclh_ObjToWideInt(Tcl_Interp *interp, Tcl_Obj *obj, Tcl_WideInt *ptr);
#else
TCLH_INLINE Tclh_ReturnCode
Tclh_ObjToWideInt(Tcl_Interp *interp, Tcl_Obj *obj, Tcl_WideInt *ptr) {
    return Tcl_GetWideIntFromObj(interp, obj, ptr);
}
#endif

/* Function: Tclh_ObjFromWideInt
 * Returns a Tcl_Obj wrapping a *Tcl_WideInt*
 *
 * Parameters:
 *  wide - Tcl_WideInt value to be wrapped
 *
 * Returns:
 * A pointer to a Tcl_Obj with a zero reference count.
 */
TCLH_INLINE Tcl_Obj *Tclh_ObjFromWideInt(Tcl_WideInt wide) {
    return Tcl_NewWideIntObj(wide);
}

/* Function: Tclh_ObjToLongLong
 * Unwraps a Tcl_Obj into a C *long long* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *long* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_INLINE int
Tclh_ObjToLongLong(Tcl_Interp *interp, Tcl_Obj *objP, signed long long *llP)
{
    /* TBD - does not check for positive overflow etc. */
    TCLH_ASSERT(sizeof(Tcl_WideInt) == sizeof(signed long long));
    return Tclh_ObjToWideInt(interp, objP, (Tcl_WideInt *)llP);
}


/* Function: Tclh_ObjToULongLong
 * Unwraps a Tcl_Obj into a C *unsigned long long* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *long* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
#ifndef TCLH_TCL87API
TCLH_LOCAL Tclh_ReturnCode
Tclh_ObjToULongLong(Tcl_Interp *interp, Tcl_Obj *obj, unsigned long long *ptr);
#else
TCLH_INLINE Tclh_ReturnCode
Tclh_ObjToULongLong(Tcl_Interp *interp, Tcl_Obj *obj, unsigned long long *ptr) {
    return Tcl_GetWideUIntFromObj(interp, obj, ptr);
}
#endif

/* Function: Tclh_ObjFromULongLong
 * Returns a Tcl_Obj wrapping a *unsigned long long*
 *
 * Parameters:
 *  ull - unsigned long long value to be wrapped
 *
 * Returns:
 * A pointer to a Tcl_Obj with a zero reference count.
 */
TCLH_LOCAL Tcl_Obj *Tclh_ObjFromULongLong(unsigned long long ull);


/* Function: Tclh_ObjToFloat
 * Unwraps a Tcl_Obj into a C *float* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *float* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ObjToFloat(Tcl_Interp *interp,
                                           Tcl_Obj *obj,
                                           float *ptr);

/* Function: Tclh_ObjToDouble
 * Unwraps a Tcl_Obj into a C *double* value type.
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the number
 * ptr - location to store extracted number
 *
 * Returns:
 * Returns TCL_OK and stores the value in location pointed to by *ptr* if the
 * passed Tcl_Obj contains an integer that fits in a C *double* type. Otherwise
 * returns TCL_ERROR with an error message in the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ObjToDouble(Tcl_Interp *interp,
                                            Tcl_Obj *obj,
                                            double *ptr);

/* Function: Tclh_ObjGetBytesByRef
 * Retrieves a reference to the byte array in a Tcl_Obj.
 *
 * Primary purpose is to hide Tcl version differences.
 *
 * Parameters:
 * interp - Interpreter for error messages. May be NULL.
 * obj - Tcl_Obj containing the bytes
 * lenPtr - location to store number of bytes in the array. May be NULL.
 *
 * Returns:
 * On success, returns a pointer to internal byte array and stores the
 * length of the array in lenPtr if not NULL. On error, returns
 * a NULL pointer with an error message in the interpreter.
 */
TCLH_INLINE char *
Tclh_ObjGetBytesByRef(Tcl_Interp *interp, Tcl_Obj *obj, Tcl_Size *lenPtr)
{
#ifdef TCLH_TCL87API
    return (char *)Tcl_GetBytesFromObj(interp, obj, lenPtr);
#else
    return (char *) Tcl_GetByteArrayFromObj(obj, lenPtr);
#endif
}

/* Function: Tclh_ObjFromAddress
 * Wraps a memory address into a Tcl_Obj.
 *
 * Parameters:
 * address - a memory address
 *
 * The *Tcl_Obj* is created as a formatted string with address in
 * hexadecimal format. The reference count on the returned *Tcl_Obj* is
 * *0* just like the standard *Tcl_Obj* creation functions.
 *
 * Returns:
 * Pointer to the created Tcl_Obj.
 */
TCLH_LOCAL Tcl_Obj *Tclh_ObjFromAddress (void *address);

/* Function: Tclh_ObjToAddress
 * Unwraps a Tcl_Obj into a memory address
 *
 * Parameters:
 * interp - Interpreter
 * obj - Tcl_Obj from which to extract the adddress
 * ptr - location to store extracted address
 *
 * Returns:
 * Returns TCL_OK on success and stores the value in location
 * pointed to by *ptr*. Otherwise returns TCL_ERROR with an error message in
 * the interpreter.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_ObjToAddress(Tcl_Interp *interp, Tcl_Obj *obj, void **ptr);

/* Function: Tclh_ObjArrayIncrRefs
 * Increments reference counts of all elements of a *Tcl_Obj** array
 *
 * Parameters:
 * objc - Number of elements in objv
 * objv - array of Tcl_Obj pointers
 */
TCLH_INLINE void Tclh_ObjArrayIncrRefs(int objc, Tcl_Obj * const *objv) {
    int i;
    for (i = 0; i < objc; ++i)
        Tcl_IncrRefCount(objv[i]);
}

/* Function: Tclh_ObjArrayDecrRefs
 * Decrements reference counts of all elements of a *Tcl_Obj** array
 *
 * Parameters:
 * objc - Number of elements in objv
 * objv - array of Tcl_Obj pointers
 */
TCLH_INLINE void Tclh_ObjArrayDecrRefs(int objc, Tcl_Obj * const *objv) {
    int i;
    for (i = 0; i < objc; ++i)
        Tcl_DecrRefCount(objv[i]);
}

/* Function: Tclh_ObjFromDString
 * Returns a Tcl_Obj from a Tcl_DString content
 *
 * Parameters:
 * dsPtr - pointer to a Tcl_DString
 *
 * The Tcl_DString is reinitialized to empty.
 *
 * Returns:
 * A non-NULL Tcl_Obj. Panics on memory failure.
 */
#ifndef TCLH_TCL87API
Tcl_Obj * Tclh_ObjFromDString (Tcl_DString *dsP);
#else
TCLH_INLINE Tcl_Obj* Tclh_ObjFromDString(Tcl_DString *dsP) {
    return Tcl_DStringToObj(dsP);
}
#endif

#ifdef TCLH_SHORTNAMES
#define ObjClearPrt Tclh_ObjClearPtr
#define ObjToRangedInt Tclh_ObjToRangedInt
#define ObjToChar Tclh_ObjToChar
#define ObjToUChar Tclh_ObjToUChar
#define ObjToShort Tclh_ObjToShort
#define ObjToUShort Tclh_ObjToUShort
#define ObjToInt Tclh_ObjToInt
#define ObjToUInt Tclh_ObjToUInt
#define ObjToLong Tclh_ObjToLong
#define ObjToULong Tclh_ObjToULong
#define ObjToWideInt Tclh_ObjToWideInt
#define ObjToUWideInt Tclh_ObjToUWideInt
#define ObjToLongLong Tclh_ObjToLongLong
#define ObjToULongLong Tclh_ObjToULongLong
#define ObjToFloat Tclh_ObjToFloat
#define ObjToDouble Tclh_ObjToDouble
#define ObjArrayIncrRef Tclh_ObjArrayIncrRef
#define ObjArrayDecrRef Tclh_ObjArrayDecrRef
#define ObjFromAddress Tclh_ObjFromAddress
#define ObjToAddress Tclh_ObjToAddress
#define ObjGetBytesByRef Tclh_ObjGetBytesByRef
#define ObjFromDString Tclh_ObjFromDString
#endif

#ifdef TCLH_IMPL
#include "tclhObjImpl.c"
#endif


#endif /* TCLHOBJ_H */
