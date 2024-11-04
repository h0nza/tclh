/*
 * Copyright (c) 2021-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhObj.h"

static const Tcl_ObjType *gTclIntType;
static const Tcl_ObjType *gTclWideIntType;
static const Tcl_ObjType *gTclBooleanType;
static const Tcl_ObjType *gTclDoubleType;
static const Tcl_ObjType *gTclBignumType;

Tclh_ReturnCode
Tclh_ObjLibInit(Tcl_Interp *interp, Tclh_LibContext *tclhCtxP)
{
    Tcl_Obj *objP;
    int ret;

    if (tclhCtxP == NULL) {
        if (Tclh_LibInit(interp, NULL) != TCL_OK)
            return TCL_ERROR;
    }

    gTclIntType = Tcl_GetObjType("int");
    if (gTclIntType == NULL) {
        objP        = Tcl_NewIntObj(0);
        gTclIntType = objP->typePtr;
        Tcl_DecrRefCount(objP);
    }

    gTclWideIntType = Tcl_GetObjType("wideInt");
    if (gTclWideIntType == NULL) {
        objP            = Tcl_NewWideIntObj(0x100000000);
        gTclWideIntType = objP->typePtr;
        Tcl_DecrRefCount(objP);
    }
    gTclBooleanType = Tcl_GetObjType("boolean");
    if (gTclBooleanType == NULL) {
        objP = Tcl_NewBooleanObj(1);
#if TCLH_TCLAPI_VERSION >= 0x0807
        char b;
        if (Tcl_GetBoolFromObj(NULL, objP, 0, &b) == TCL_OK) {
            gTclBooleanType = objP->typePtr;
        }
#endif
        Tcl_DecrRefCount(objP);
    }
    gTclDoubleType = Tcl_GetObjType("double");
    if (gTclDoubleType == NULL) {
        objP           = Tcl_NewDoubleObj(0.1);
        gTclDoubleType = objP->typePtr;
        Tcl_DecrRefCount(objP);
    }
    gTclBignumType = Tcl_GetObjType("bignum"); /* Likely NULL as it is not registered */
    if (gTclBignumType == NULL) {
        objP    = Tcl_NewStringObj("0xffffffffffffffff", -1);
#ifdef TCLH_TCL87API
        void *mpIntPtr = NULL;
        int type;
        ret = Tcl_GetNumberFromObj(NULL, objP, &mpIntPtr, &type);
        if (ret == TCL_OK && type == TCL_NUMBER_BIG) {
            gTclBignumType = objP->typePtr;
        }
#else
        mp_int temp;
        ret = Tcl_GetBignumFromObj(NULL, objP, &temp);
        if (ret == TCL_OK) {
            gTclBignumType = objP->typePtr;
            mp_clear(&temp);
        }
#endif
        Tcl_DecrRefCount(objP);
    }

    return TCL_OK;
}

const Tcl_ObjType *
Tclh_GetObjTypeDescriptor(const char *typename)
{
    if (!strcmp(typename, "int")) {
        return gTclIntType;
    }
    if (!strcmp(typename, "wide") || !strcmp(typename, "wideInt")) {
        return gTclWideIntType;
    }
    if (!strcmp(typename, "double")) {
        return gTclDoubleType;
    }
    if (!strcmp(typename, "bool") || !strcmp(typename, "boolean")) {
        return gTclBooleanType;
    }
    if (!strcmp(typename, "bignum")) {
        return gTclBignumType;
    }
    return Tcl_GetObjType(typename);
}

Tclh_ReturnCode
Tclh_ObjToRangedInt(Tcl_Interp *interp,
                    Tcl_Obj *obj,
                    Tcl_WideInt low,
                    Tcl_WideInt high,
                    Tcl_WideInt *wideP)
{
    Tcl_WideInt wide;

    if (Tclh_ObjToWideInt(interp, obj, &wide) != TCL_OK)
        return TCL_ERROR;

    if (wide < low || wide > high)
        return Tclh_ErrorRange(interp, obj, low, high);

    if (wideP)
        *wideP = wide;

    return TCL_OK;
}

Tclh_ReturnCode
Tclh_ObjToChar(Tcl_Interp *interp, Tcl_Obj *obj, signed char *cP)
{
    Tcl_WideInt wide = 0; /* Init to keep gcc happy */

    if (Tclh_ObjToRangedInt(interp, obj, SCHAR_MIN, SCHAR_MAX, &wide) != TCL_OK)
        return TCL_ERROR;
    *cP = (signed char) wide;
    return TCL_OK;
}

Tclh_ReturnCode
Tclh_ObjToUChar(Tcl_Interp *interp, Tcl_Obj *obj, unsigned char *ucP)
{
    Tcl_WideInt wide = 0; /* Init to keep gcc happy */

    if (Tclh_ObjToRangedInt(interp, obj, 0, UCHAR_MAX, &wide) != TCL_OK)
        return TCL_ERROR;
    *ucP = (unsigned char) wide;
    return TCL_OK;
}

Tclh_ReturnCode
Tclh_ObjToShort(Tcl_Interp *interp, Tcl_Obj *obj, short *shortP)
{
    Tcl_WideInt wide = 0; /* Init to keep gcc happy */

    if (Tclh_ObjToRangedInt(interp, obj, SHRT_MIN, SHRT_MAX, &wide) != TCL_OK)
        return TCL_ERROR;
    *shortP = (short) wide;
    return TCL_OK;
}

Tclh_ReturnCode
Tclh_ObjToUShort(Tcl_Interp *interp, Tcl_Obj *obj, unsigned short *ushortP)
{
    Tcl_WideInt wide = 0; /* Init to keep gcc happy */

    if (Tclh_ObjToRangedInt(interp, obj, 0, USHRT_MAX, &wide) != TCL_OK)
        return TCL_ERROR;
    *ushortP = (unsigned short) wide;
    return TCL_OK;
}

Tclh_ReturnCode
Tclh_ObjToInt(Tcl_Interp *interp, Tcl_Obj *objP, int *valP)
{
#if 0
    /* BAD - returns 2147483648 as -2147483648 and
       -2147483649 as 2147483647 instead of error */
       /* TODO - check if Tcl9 still does this */
    return Tcl_GetIntFromObj(interp, objP, valP);
#else
    Tcl_WideInt wide = 0; /* Init to keep gcc happy */

    if (Tclh_ObjToRangedInt(interp, objP, INT_MIN, INT_MAX, &wide) != TCL_OK)
        return TCL_ERROR;
    *valP = (int) wide;
    return TCL_OK;
#endif
}

Tclh_ReturnCode
Tclh_ObjToUInt(Tcl_Interp *interp, Tcl_Obj *obj, unsigned int *uiP)
{
    Tcl_WideInt wide = 0; /* Init to keep gcc happy */

    if (Tclh_ObjToRangedInt(interp, obj, 0, UINT_MAX, &wide) != TCL_OK)
        return TCL_ERROR;
    *uiP = (unsigned int) wide;
    return TCL_OK;
}

Tclh_ReturnCode
Tclh_ObjToLong(Tcl_Interp *interp, Tcl_Obj *objP, long *valP)
{
    if (sizeof(long) < sizeof(Tcl_WideInt)) {
        Tcl_WideInt wide = 0; /* Init to keep gcc happy */
        if (Tclh_ObjToRangedInt(interp, objP, LONG_MIN, LONG_MAX, &wide) != TCL_OK)
            return TCL_ERROR;
        *valP = (long)wide;
        return TCL_OK;
    }
    else {
        TCLH_ASSERT(sizeof(long long) == sizeof(long));
        return Tclh_ObjToLongLong(interp, objP, (long long *) valP);
    }
}

Tclh_ReturnCode
Tclh_ObjToULong(Tcl_Interp *interp, Tcl_Obj *objP, unsigned long *valP)
{

    if (sizeof(unsigned long) < sizeof(Tcl_WideInt)) {
        Tcl_WideInt wide = 0; /* Init to keep gcc happy */
        if (Tclh_ObjToRangedInt(interp, objP, 0, ULONG_MAX, &wide) != TCL_OK)
            return TCL_ERROR;
        *valP = (unsigned long)wide;
        return TCL_OK;
    }
    else {
        /* TBD - unsigned not handled correctly */
        TCLH_ASSERT(sizeof(unsigned long long) == sizeof(unsigned long));
        return Tclh_ObjToULongLong(interp, objP, (unsigned long long *) valP);
    }
}

Tcl_Obj *Tclh_ObjFromULong(unsigned long ul)
{
    if (sizeof(unsigned long) < sizeof(Tcl_WideInt))
        return Tcl_NewWideIntObj(ul);
    else
        return Tclh_ObjFromULongLong(ul);
}

#ifndef TCLH_TCL87API
Tclh_ReturnCode
Tclh_ObjToWideInt(Tcl_Interp *interp, Tcl_Obj *objP, Tcl_WideInt *wideP)
{
    /*
     * Tcl_GetWideIntFromObj has several issues on Tcl 8.6:
     * - it will happily accept negative numbers in strings that will
     *   then show up as positive when retrieved. For example,
     *   Tcl_GWIFO(Tcl_NewStringObj(-18446744073709551615, -1)) will
     *   return a value of 1.
     *   Similarly -9223372036854775809 -> large positive number
     * - It will also happily accept valid positive numbers in the range
     *   LLONG_MAX - ULLONG_MAX but return them as negative numbers which we
     *   cannot distinguish from genuine negative numbers
     * So we check the internal rep. If it is an integer type other than
     * bignum, fine. Otherwise, we check for possibility of overflow by
     * comparing sign of retrieved wide int with the sign stored in the
     * bignum representation.
     */
    int ret;
    Tcl_WideInt wide;
    ret = Tcl_GetWideIntFromObj(interp, objP, &wide);
    if (ret != TCL_OK)
        return ret;

    if (objP->typePtr != gTclIntType && objP->typePtr != gTclWideIntType
        && objP->typePtr != gTclBooleanType
        && objP->typePtr != gTclDoubleType) {
        /* Was it an integer overflow */
        mp_int temp;
        ret = Tcl_GetBignumFromObj(interp, objP, &temp);
        if (ret == TCL_OK) {
            if ((wide >= 0 && temp.sign == MP_NEG)
                || (wide < 0 && temp.sign != MP_NEG)) {
                Tcl_SetResult(interp,
                              "Integer magnitude too large to represent.",
                              TCL_STATIC);
                ret = TCL_ERROR;
            }
            mp_clear(&temp);
        }
    }

    if (ret == TCL_OK)
        *wideP = wide;
    return ret;
}
#endif

#ifndef TCLH_TCL87API
Tclh_ReturnCode
Tclh_ObjToULongLong(Tcl_Interp *interp, Tcl_Obj *objP, unsigned long long *ullP)
{
    int ret;
    Tcl_WideInt wide;

    TCLH_ASSERT(sizeof(unsigned long long) == sizeof(Tcl_WideInt));

    /* Tcl_GetWideInt will happily return overflows as negative numbers */
    ret = Tcl_GetWideIntFromObj(interp, objP, &wide);
    if (ret != TCL_OK)
        return ret;

    /*
     * We have to check for two things.
     *   1. an overflow condition in Tcl_GWIFO where
     *     (a) a large positive number that fits in the width is returned
     *         as negative e.g. 18446744073709551615 is returned as -1
     *     (b) an negative overflow is returned as a positive number,
     *         e.g. -18446744073709551615 is returned as 1.
     *   2. Once we have retrieved a valid number, reject it if negative.
     *
     * So we check the internal rep. If it is an integer type other than
     * bignum, fine (no overflow). Otherwise, we check for possibility of
     * overflow by comparing sign of retrieved wide int with the sign stored
     * in the bignum representation.
     */

    if (objP->typePtr == gTclIntType || objP->typePtr == gTclWideIntType
        || objP->typePtr == gTclBooleanType
        || objP->typePtr == gTclDoubleType) {
        /* No need for an overflow check (1) but still need to check (2) */
        if (wide < 0)
            goto negative_error;
        *ullP = (unsigned long long)wide;
    }
    else {
        /* Was it an integer overflow */
        mp_int temp;
        ret = Tcl_GetBignumFromObj(interp, objP, &temp);
        if (ret == TCL_OK) {
            int sign = temp.sign;
            mp_clear(&temp);
            if (sign == MP_NEG)
                goto negative_error;
            /*
             * Note Tcl_Tcl_GWIFO already takes care of overflows that do not
             * fit in Tcl_WideInt width. So we need not worry about that.
             * The overflow case is where a positive value is returned as
             * negative by Tcl_GWIFO; that is also taken care of by the
             * assignment below.
             */
            *ullP = (unsigned long long)wide;
        }
    }

    return ret;

negative_error:
    return TclhRecordError(
        interp,
        "RANGE",
        Tcl_NewStringObj("Negative values are not in range for unsigned types.", -1));
}
#endif

Tcl_Obj *Tclh_ObjFromULongLong(unsigned long long ull)
{
    /* TODO - see how TIP 648 does it */
    TCLH_ASSERT(sizeof(Tcl_WideInt) == sizeof(unsigned long long));
    if (ull <= LLONG_MAX)
        return Tcl_NewWideIntObj((Tcl_WideInt) ull);
    else {
        /* Cannot use WideInt because that will treat as negative  */
        char buf[40]; /* Think 21 enough, but not bothered to count */
#ifdef _WIN32
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%I64u", ull);
#else
        snprintf(buf, sizeof(buf), "%llu", ull);
#endif
        return Tcl_NewStringObj(buf, -1);
    }
}

Tclh_ReturnCode
Tclh_ObjToDouble(Tcl_Interp *interp, Tcl_Obj *objP, double *dblP)
{
    return Tcl_GetDoubleFromObj(interp, objP, dblP);
}

Tclh_ReturnCode
Tclh_ObjToFloat(Tcl_Interp *interp, Tcl_Obj *objP, float *fltP)
{
    double dval;
    if (Tcl_GetDoubleFromObj(interp, objP, &dval) != TCL_OK)
        return TCL_ERROR;
    *fltP = (float) dval;
    return TCL_OK;
}

Tcl_Obj *Tclh_ObjFromAddress (void *address)
{
    char buf[40];
    char *start;

    start = TclhPrintAddress(address, buf, sizeof(buf) / sizeof(buf[0]));
    return Tcl_NewStringObj(start, -1);
}

Tclh_ReturnCode
Tclh_ObjToAddress(Tcl_Interp *interp, Tcl_Obj *objP, void **pvP)
{
    int ret;

    /* Yeah, assumes pointers are 4 or 8 bytes only */
    if (sizeof(unsigned int) == sizeof(*pvP)) {
        unsigned int ui;
        ret = Tclh_ObjToUInt(interp, objP, &ui);
        if (ret == TCL_OK)
            *pvP = (void *)(uintptr_t)ui;
    }
    else {
        Tcl_WideInt wide;
        ret = Tcl_GetWideIntFromObj(interp, objP, &wide);
        if (ret == TCL_OK)
            *pvP = (void *)(uintptr_t)wide;
    }
    return ret;
}

#ifndef TCLH_TCL87API
/* Copied from Tcl 8.7+ as polyfill for 8.6 */
Tcl_Obj* Tclh_ObjFromDString(Tcl_DString *dsPtr) 
{
    Tcl_Obj *result;

    if (dsPtr->string == dsPtr->staticSpace) {
        if (dsPtr->length == 0) {
            result = Tcl_NewObj();
        }
        else {
            /* Static buffer, so must copy. */
            result = Tcl_NewStringObj(dsPtr->string, dsPtr->length);
        }
    }
    else {
        /* Dynamic buffer, so transfer ownership and reset. */
        result = Tcl_NewObj();
        result->bytes  = dsPtr->string;
        result->length = dsPtr->length;
    }

    /* Re-establish the DString as empty with no buffer allocated. */

    dsPtr->string = dsPtr->staticSpace;
    dsPtr->spaceAvl = TCL_DSTRING_STATIC_SIZE;
    dsPtr->length = 0;
    dsPtr->staticSpace[0] = '\0';

    return result;

}
#endif /* TCLH_TCL87API */