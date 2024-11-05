/*
 * Copyright (c) 2021-2024, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhPointer.h"
#include <stdarg.h>

/*
 * Pointer registry implementation.
 */

typedef enum Tclh_PointerRegistrationType {
    TCLH_UNCOUNTED_POINTER,
    TCLH_COUNTED_POINTER,
    TCLH_PINNED_POINTER
} Tclh_PointerRegistrationType;


/*
 * TclhPointerRecord keeps track of pointers and the count of references
 * to them. Pointers that are single reference have a nRefs of -1.
 */
typedef struct TclhPointerRecord {
    Tcl_Obj *tagObj;            /* Identifies the "type". May be NULL */
    int nRefs;                  /* Number of references to the pointer */
#define TCLH_POINTER_NREFS_MAX INT_MAX
} TclhPointerRecord;

typedef struct TclhPointerRegistry {
    Tcl_HashTable pointers;/* Table of registered pointers */
    Tcl_HashTable castables;/* Table of permitted casts subclass -> class */
} TclhPointerRegistry;

/*
 * Pointer is a Tcl "type" whose internal representation is stored
 * as the pointer value and an associated C pointer/handle type.
 * The Tcl_Obj.internalRep.twoPtrValue.ptr1 holds the C pointer value
 * and Tcl_Obj.internalRep.twoPtrValue.ptr2 holds a Tcl_Obj describing
 * the type. This may be NULL if no type info needs to be associated
 * with the value.
 */
static void DupPointerType(Tcl_Obj *srcP, Tcl_Obj *dstP);
static void FreePointerType(Tcl_Obj *objP);
static void UpdatePointerTypeString(Tcl_Obj *objP);
static int  SetPointerFromAny(Tcl_Interp *interp, Tcl_Obj *objP);

static struct Tcl_ObjType gPointerType = {
    "Pointer",
    FreePointerType,
    DupPointerType,
    UpdatePointerTypeString,
    NULL,
};
TCLH_INLINE void *PointerValueGet(Tcl_Obj *objP) {
    return objP->internalRep.twoPtrValue.ptr1;
}
TCLH_INLINE void PointerValueSet(Tcl_Obj *objP, void *valueP) {
    objP->internalRep.twoPtrValue.ptr1 = valueP;
}
/* May return NULL */
TCLH_INLINE Tclh_PointerTypeTag PointerTypeGet(Tcl_Obj *objP) {
    return objP->internalRep.twoPtrValue.ptr2;
}
TCLH_INLINE void PointerTypeSet(Tcl_Obj *objP, Tclh_PointerTypeTag tag) {
    objP->internalRep.twoPtrValue.ptr2 = (void*)tag;
}
static int PointerTypeMatchesExpected(Tclh_PointerTypeTag pointer_tag,
                           Tclh_PointerTypeTag expected_tag);
static int PointerTypeCompatible(TclhPointerRegistry *registryP,
                                 Tclh_PointerTypeTag tag,
                                 Tclh_PointerTypeTag expected);
static void TclhPointerRecordFree(TclhPointerRecord *ptrRecP);

static void
TclhCleanupPointerRegistry(ClientData clientData, Tcl_Interp *interp)
{
    TclhPointerRegistry *registryP = (TclhPointerRegistry *)clientData;
    TCLH_ASSERT(registryP);

    Tcl_HashTable *hTblPtr;
    Tcl_HashEntry *he;
    Tcl_HashSearch hSearch;
    hTblPtr = &registryP->pointers;
    for (he = Tcl_FirstHashEntry(hTblPtr, &hSearch); he != NULL;
         he = Tcl_NextHashEntry(&hSearch)) {
        TclhPointerRecordFree((TclhPointerRecord *)Tcl_GetHashValue(he));
    }
    Tcl_DeleteHashTable(&registryP->pointers);

    hTblPtr = &registryP->castables;
    for (he = Tcl_FirstHashEntry(hTblPtr, &hSearch); he != NULL;
         he = Tcl_NextHashEntry(&hSearch)) {
        Tcl_Obj *superTagObj = (Tcl_Obj *)Tcl_GetHashValue(he);
        /* Future proof in case we allow superTagObj as NULL */
        if (superTagObj)
            Tcl_DecrRefCount(superTagObj);
    }
    Tcl_DeleteHashTable(&registryP->castables);

    Tclh_Free(registryP);
}

static TclhPointerRegistry *
TclhPointerGetRegistry(Tcl_Interp *interp, Tclh_LibContext *tclhCtxP)
{
    if (tclhCtxP == NULL) {
        if (interp == NULL || Tclh_LibInit(interp, &tclhCtxP) != TCL_OK)
            return NULL;
    }
    if (tclhCtxP->pointerRegistryP == NULL) {
        (void) Tclh_ErrorGeneric(
            interp, NULL, "Internal error: Tclh context not initialized.");
        return NULL;
    }
    return tclhCtxP->pointerRegistryP;
}


Tclh_ReturnCode
Tclh_PointerLibInit(Tcl_Interp *interp, Tclh_LibContext *tclhCtxP)
{
    Tclh_ReturnCode ret;

    if (tclhCtxP == NULL) {
        if (interp == NULL)
            return TCL_ERROR;
        ret = Tclh_LibInit(interp, &tclhCtxP);
        if (ret != TCL_OK)
            return ret;
    }

    if (tclhCtxP->pointerRegistryP)
        return TCL_OK; /* Already done */

    TclhPointerRegistry *registryP;
    registryP = (TclhPointerRegistry *)Tcl_Alloc(sizeof(*registryP));
    Tcl_InitHashTable(&registryP->pointers, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&registryP->castables, TCL_STRING_KEYS);
    Tcl_CallWhenDeleted(interp, TclhCleanupPointerRegistry, registryP);
    tclhCtxP->pointerRegistryP = registryP;

    return TCL_OK;
}

Tclh_ReturnCode
Tclh_ErrorPointerNull(Tcl_Interp *interp)
{
    return Tclh_ErrorInvalidValue(interp, NULL, "Pointer is NULL.");
}

Tclh_ReturnCode
Tclh_ErrorPointerObjType(Tcl_Interp *interp,
                      Tcl_Obj *ptrObj,
                      Tclh_PointerTypeTag tag)
{
    char buf[200];
    snprintf(buf,
             sizeof(buf),
             "Expected pointer to %s.",
             tag ? Tcl_GetString(tag) : "void");
    return Tclh_ErrorWrongType(interp, ptrObj, buf);
}

const char *RegistrationStatusToString(
                              Tclh_PointerRegistrationStatus regStatus)
{
    switch (regStatus) {
    case TCLH_POINTER_REGISTRATION_MISSING:
        return "Pointer validation failed: not registered.";
    case TCLH_POINTER_REGISTRATION_WRONGTAG:
        return "Pointer validation failed: type does not match registration.";
    case TCLH_POINTER_REGISTRATION_DERIVED:
    case TCLH_POINTER_REGISTRATION_OK:
    default:
        /* Should really not happen */
        return "Pointer validation failed.";
    }
}

static Tclh_ReturnCode
PointerNotRegisteredError(Tcl_Interp *interp,
                          const void *p,
                          Tclh_PointerTypeTag tag,
                          Tclh_PointerRegistrationStatus regStatus)
{
    char addr[40];
    TclhPrintAddress(p, addr, sizeof(addr));
    char buf[200];
    snprintf(buf, sizeof(buf), "%s^%s", addr, tag ? Tcl_GetString(tag) : "");

    return Tclh_ErrorInvalidValueStr(
        interp, buf, RegistrationStatusToString(regStatus));
}

Tclh_ReturnCode
Tclh_ErrorPointerObjRegistration(Tcl_Interp *interp,
                              Tcl_Obj *ptrObj,
                              Tclh_PointerRegistrationStatus regStatus)
{
    return Tclh_ErrorInvalidValue(
        interp, ptrObj, RegistrationStatusToString(regStatus));
}

static Tclh_ReturnCode
PointerTypeMismatchError(Tcl_Interp *interp,
                 Tclh_PointerTypeTag tag,
                 Tclh_PointerTypeTag expected)
{
    char buf[200];
    snprintf(buf,
             sizeof(buf),
             "Expected pointer^%s, got pointer^%s.",
             expected ? Tcl_GetString(expected) : "",
             tag ? Tcl_GetString(tag) : "");
    return Tclh_ErrorWrongType(interp, NULL, buf);
}

static int
PointerTypeMatchesExpected(Tclh_PointerTypeTag pointer_tag,
                Tclh_PointerTypeTag expected_tag)
{
    if (pointer_tag == expected_tag || expected_tag == NULL)
        return 1;

    if (pointer_tag == NULL)
        return 0;

    return !strcmp(Tcl_GetString(pointer_tag), Tcl_GetString(expected_tag));
}


Tcl_Obj *
Tclh_PointerWrap(void *pointerValue, Tclh_PointerTypeTag tag)
{
    Tcl_Obj *objP;

    objP = Tcl_NewObj();
    Tcl_InvalidateStringRep(objP);
    PointerValueSet(objP, pointerValue);
    if (tag)
        Tcl_IncrRefCount(tag);
    PointerTypeSet(objP, tag);
    objP->typePtr = &gPointerType;
    return objP;
}

Tclh_ReturnCode
Tclh_PointerUnwrap(Tcl_Interp *interp, Tcl_Obj *objP, void **pvP)
{
    /* Try converting Tcl_Obj internal rep */
    if (objP->typePtr != &gPointerType) {
        if (SetPointerFromAny(interp, objP) != TCL_OK)
            return TCL_ERROR;
    }

    *pvP = PointerValueGet(objP);
    return TCL_OK;
}

Tclh_ReturnCode
Tclh_PointerUnwrapTagged(Tcl_Interp *interp,
                         Tclh_LibContext *tclhCtxP,
                         Tcl_Obj *objP,
                         void **pvP,
                         Tclh_PointerTypeTag *tagP,
                         Tclh_PointerTypeTag expectedTag)
{
    Tclh_PointerTypeTag tag;
    void *pv;

    /* Try converting Tcl_Obj internal rep */
    if (objP->typePtr != &gPointerType) {
        if (SetPointerFromAny(interp, objP) != TCL_OK)
            return TCL_ERROR;
    }

    tag = PointerTypeGet(objP);
    pv  = PointerValueGet(objP);

    /*
    * Check tag if
    *   - expectedTag is not NULL,
    *     and
    *   - the unwrapped pointer is not NULL or its tag is not NULL
    */
    if (expectedTag && (pv || tag) && tag != expectedTag) {
        TclhPointerRegistry *registryP =
            TclhPointerGetRegistry(interp, tclhCtxP);
        if (registryP == NULL)
            return TCL_ERROR;

        if (!PointerTypeCompatible(registryP, tag, expectedTag)) {
            return Tclh_ErrorPointerObjType(interp, objP, expectedTag);
        }
    }

    if (pvP)
        *pvP = PointerValueGet(objP);
    if (tagP)
        *tagP = tag;
    return TCL_OK;
}

Tclh_ReturnCode
Tclh_PointerObjGetTag(Tcl_Interp *interp,
                      Tcl_Obj *objP,
                      Tclh_PointerTypeTag *tagPtr)
{
    /* Try converting Tcl_Obj internal rep */
    if (objP->typePtr != &gPointerType) {
        if (SetPointerFromAny(interp, objP) != TCL_OK)
            return TCL_ERROR;
    }
    *tagPtr = PointerTypeGet(objP);
    return TCL_OK;
}

Tclh_ReturnCode
Tclh_PointerObjCompare(Tcl_Interp *interp,
                       Tcl_Obj *ptr1Obj,
                       Tcl_Obj *ptr2Obj,
                       int *resultP)
{
    void *ptr1, *ptr2;
    TCLH_CHECK_RESULT(Tclh_PointerUnwrap(interp, ptr1Obj, &ptr1));
    TCLH_CHECK_RESULT(Tclh_PointerUnwrap(interp, ptr2Obj, &ptr2));
    if (ptr1 != ptr2) {
        *resultP = 0;
        return TCL_OK;
    }

    Tclh_PointerTypeTag tag1;
    Tclh_PointerTypeTag tag2;
    TCLH_CHECK_RESULT(Tclh_PointerObjGetTag(interp, ptr1Obj, &tag1));
    TCLH_CHECK_RESULT(Tclh_PointerObjGetTag(interp, ptr2Obj, &tag2));
    if (tag1 == tag2) {
        *resultP = 1;
        return TCL_OK;
    }
    if (tag1 == NULL || tag2 == NULL) {
        *resultP = -1;
        return TCL_OK;
    }
    *resultP = strcmp(Tcl_GetString(tag1), Tcl_GetString(tag2)) ? -1 : 1;
    return TCL_OK;
}

static Tclh_ReturnCode
TclhUnwrapAnyOfVA(Tcl_Interp *interp,
                  Tclh_LibContext *tclhCtxP,
                  Tcl_Obj *objP,
                  void **pvP,
                  Tclh_PointerTypeTag *tagP,
                  va_list args)
{
    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP == NULL)
        return TCL_ERROR;
    TCLH_ASSERT(registryP == tclhCtxP->pointerRegistryP);

    Tclh_PointerTypeTag tag;
    Tclh_PointerTypeTag lastTag = NULL;
    while ((tag = va_arg(args, Tclh_PointerTypeTag)) != NULL) {
        /* Pass in tclhCtxP, not interp to avoid interp error message */
        lastTag = tag;
        if (Tclh_PointerUnwrapTagged(NULL, tclhCtxP, objP, pvP, tagP, tag) == TCL_OK) {
            return TCL_OK;
        }
    }

    return Tclh_ErrorPointerObjType(interp, objP, lastTag);
}

Tclh_ReturnCode
Tclh_PointerUnwrapAnyOf(Tcl_Interp *interp,
                        Tclh_LibContext *tclhCtxP,
                        Tcl_Obj *objP,
                        void **pvP,
                        ...)
{
    int     tclResult;
    va_list args;

    va_start(args, pvP);
    tclResult = TclhUnwrapAnyOfVA(interp, tclhCtxP, objP, pvP, NULL, args);
    va_end(args);
    return tclResult;
}

static void
UpdatePointerTypeString(Tcl_Obj *objP)
{
    Tclh_PointerTypeTag tagObj;
    Tcl_Size len;
    Tcl_Size taglen;
    char *tagStr;
    char *bytes;

    TCLH_ASSERT(objP->bytes == NULL);
    TCLH_ASSERT(objP->typePtr == &gPointerType);

    tagObj = PointerTypeGet(objP);
    if (tagObj) {
        tagStr = Tcl_GetStringFromObj(tagObj, &taglen);
    }
    else {
        tagStr = "";
        taglen = 0;
    }
    /* Assume 40 bytes enough for address */
    bytes = (char *) Tcl_Alloc(40 + 1 + taglen + 1);
    (void) TclhPrintAddress(PointerValueGet(objP), bytes, 40);
    len = Tclh_strlen(bytes);
    bytes[len] = '^';
    memcpy(bytes + len + 1, tagStr, taglen+1);
    objP->bytes = bytes;
    objP->length = len + 1 + taglen;
}

static void
FreePointerType(Tcl_Obj *objP)
{
    Tclh_PointerTypeTag tag = PointerTypeGet(objP);
    if (tag)
        Tcl_DecrRefCount(tag);
    PointerTypeSet(objP, NULL);
    PointerValueSet(objP, NULL);
    objP->typePtr = NULL;
}

static void
DupPointerType(Tcl_Obj *srcP, Tcl_Obj *dstP)
{
    Tclh_PointerTypeTag tag;
    dstP->typePtr = &gPointerType;
    PointerValueSet(dstP, PointerValueGet(srcP));
    tag = PointerTypeGet(srcP);
    if (tag)
        Tcl_IncrRefCount(tag);
    PointerTypeSet(dstP, tag);
}

static int
SetPointerFromAny(Tcl_Interp *interp, Tcl_Obj *objP)
{
    void *pv;
    Tclh_PointerTypeTag tagObj;
    char *srep;
    int nfields;
    char terminator;
    int consumed;

    if (objP->typePtr == &gPointerType)
        return TCL_OK;

    /* Pointers are address^tag, NULL*/
    srep = Tcl_GetString(objP);
    if (sizeof(void*) == sizeof(unsigned int)) {
        unsigned int ui;
        nfields = sscanf(srep, "0x%x%c%n", &ui, &terminator, &consumed);
        pv      = (void *)(uintptr_t)ui; /* No matter if scan failed */
    }
    else {
        unsigned long long ull;
        nfields = sscanf(srep, "0x%llx%c%n", &ull, &terminator, &consumed);
        pv      = (void *)(uintptr_t)ull; /* No matter if scan failed */
    }
    if (nfields == 2 && terminator == '^') {
        if (srep[consumed] == '\0')
            tagObj = NULL;
        else {
            tagObj = Tcl_NewStringObj(srep+consumed, -1);
            Tcl_IncrRefCount(tagObj);
        }
    }
    else {
        if (strcmp(srep, "NULL"))
            goto invalid_value;
        pv = NULL;
        tagObj = NULL;
    }

    /* OK, valid opaque rep. Convert the passed object's internal rep */
    if (objP->typePtr && objP->typePtr->freeIntRepProc) {
        objP->typePtr->freeIntRepProc(objP);
    }
    objP->typePtr = &gPointerType;
    PointerValueSet(objP, pv);
    PointerTypeSet(objP, tagObj);

    return TCL_OK;

invalid_value: /* s must point to value */
    return Tclh_ErrorInvalidValue(interp, objP, "Invalid pointer format.");
}

static void
TclhPointerRecordFree(TclhPointerRecord *ptrRecP)
{
    /* TBD - this assumes pointer tags are tagObj */
    if (ptrRecP->tagObj)
        Tcl_DecrRefCount(ptrRecP->tagObj);
    Tclh_Free(ptrRecP);
}

Tclh_ReturnCode
TclhPointerRegister(Tcl_Interp *interp,
                    Tclh_LibContext *tclhCtxP,
                    void *pointer,
                    Tclh_PointerTypeTag tag,
                    Tcl_Obj **objPP,
                    Tclh_PointerRegistrationType registration)
{
    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP == NULL)
        return TCL_ERROR;

    Tcl_HashTable *hTblPtr;
    Tcl_HashEntry *he;
    int            newEntry;
    TclhPointerRecord *ptrRecP;

    if (pointer == NULL)
        return Tclh_ErrorPointerNull(interp);

    hTblPtr   = &registryP->pointers;
    he = Tcl_CreateHashEntry(hTblPtr, pointer, &newEntry);

    if (he) {
        if (newEntry) {
            ptrRecP = (TclhPointerRecord *)Tcl_Alloc(sizeof(*ptrRecP));
            if (tag && registration != TCLH_PINNED_POINTER) {
                Tcl_IncrRefCount(tag);
                ptrRecP->tagObj = tag;
            }
            else
                ptrRecP->tagObj = NULL;
            /* -1 => uncounted pointer (only single reference allowed) */
            switch (registration) {
                case TCLH_UNCOUNTED_POINTER:
                    ptrRecP->nRefs = -1;
                    break;
                case TCLH_COUNTED_POINTER:
                    ptrRecP->nRefs = 1;
                    break;
                case TCLH_PINNED_POINTER:
                    ptrRecP->nRefs = TCLH_POINTER_NREFS_MAX;
                    break;
            }
            Tcl_SetHashValue(he, ptrRecP);
        } else {
            ptrRecP = Tcl_GetHashValue(he);
            /* Note pinned pointers are unaffected */
            if (ptrRecP->nRefs != TCLH_POINTER_NREFS_MAX) {
                /*
                 * If new registration is pinned, it takes precedence
                 * Tags are immaterial in this case
                 */
                if (registration == TCLH_PINNED_POINTER) {
                    if (ptrRecP->tagObj) {
                        Tcl_DecrRefCount(ptrRecP->tagObj);
                        ptrRecP->tagObj = NULL;
                    }
                    ptrRecP->nRefs = TCLH_POINTER_NREFS_MAX;
                }
                else {
                    /* If the existing tag is compatible AND registration type is same
                        */
                    int tagCompatible =
                        PointerTypeCompatible(registryP, tag, ptrRecP->tagObj);
                    if (tagCompatible && registration == TCLH_UNCOUNTED_POINTER && ptrRecP->nRefs < 0) {
                        /* Tag compatible and both are uncounted */
                        /* Nothing to do, ref count unchanged */
                    }
                    else if (tagCompatible
                             && registration == TCLH_COUNTED_POINTER
                             && ptrRecP->nRefs > 0) {
                        /* Tag compatible and counted */
                        ptrRecP->nRefs += 1;
                    }
                    else {
                        /* Incompatible tags or different registration. Overwrite */
                        if (tag)
                            Tcl_IncrRefCount(tag);
                        if (ptrRecP->tagObj)
                            Tcl_IncrRefCount(ptrRecP->tagObj);
                        ptrRecP->tagObj = tag;
                        ptrRecP->nRefs =
                            registration == TCLH_UNCOUNTED_POINTER ? -1 : 1;
                    }
                }
            }
        }
        if (objPP)
            *objPP = Tclh_PointerWrap(pointer, tag);
    } else {
        TCLH_PANIC("Failed to allocate hash table entry.");
        return TCL_ERROR; /* NOT REACHED but keep compiler happy */
    }
    return TCL_OK;
}

Tclh_ReturnCode
Tclh_PointerRegister(Tcl_Interp *interp,
                     Tclh_LibContext *tclhCtxP,
                     void *pointer,
                     Tclh_PointerTypeTag tag,
                     Tcl_Obj **objPP)
{
    return TclhPointerRegister(
        interp, tclhCtxP, pointer, tag, objPP, TCLH_UNCOUNTED_POINTER);
}

Tclh_ReturnCode
Tclh_PointerRegisterCounted(Tcl_Interp *interp,
                            Tclh_LibContext *tclhCtxP,
                            void *pointer,
                            Tclh_PointerTypeTag tag,
                            Tcl_Obj **objPP)
{
    return TclhPointerRegister(
        interp, tclhCtxP, pointer, tag, objPP, TCLH_COUNTED_POINTER);
}

Tclh_ReturnCode
Tclh_PointerRegisterPinned(Tcl_Interp *interp,
                Tclh_LibContext *tclhCtxP,
                void *pointer,
                Tclh_PointerTypeTag tag,
                Tcl_Obj **objPP)
{
    return TclhPointerRegister(
        interp, tclhCtxP, pointer, tag, objPP, TCLH_PINNED_POINTER);
}

static int
PointerTypeCompatible(TclhPointerRegistry *registryP,
                      Tclh_PointerTypeTag tag,
                      Tclh_PointerTypeTag expected)
{
    int i;

    /* Rather than trying to detect loops by recording history
     * just keep a hard limit 10 on the depth of lookups
     */
    /* NOTE: on first go around ok for tag to be NULL. */
    if (PointerTypeMatchesExpected(tag, expected))
        return 1;
    /* For NULL, if first did not match succeeding will not either  */
    if (tag == NULL)
        return 0;
    for (i = 0; i < 10; ++i) {
        Tcl_HashEntry *he;
        he = Tcl_FindHashEntry(&registryP->castables, Tcl_GetString(tag));
        if (he == NULL)
            return 0;/* No super type */
        tag = Tcl_GetHashValue(he);
        /* For NULL, if first did not match succeeding will not either cut short */
        if (tag == NULL)
            return 0;
        if (PointerTypeMatchesExpected(tag, expected))
            return 1;
    }

    return 0;
}

Tclh_ReturnCode
Tclh_PointerUnregister(Tcl_Interp *interp,
                       Tclh_LibContext *tclhCtxP,
                       const void *pointer)
{
    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP == NULL)
        return TCL_ERROR;

    Tcl_HashEntry *he;

    he = Tcl_FindHashEntry(&registryP->pointers, pointer);
    if (he) {
        TclhPointerRecord *ptrRecP = Tcl_GetHashValue(he);
        /* Pinned pointers stay pinned */
        if (ptrRecP->nRefs != TCLH_POINTER_NREFS_MAX) {
            if (ptrRecP->nRefs <= 1) {
                TclhPointerRecordFree(ptrRecP);
                Tcl_DeleteHashEntry(he);
            }
        else {
            ptrRecP->nRefs -= 1;
        }
        }
        return TCL_OK;
    }
    else
        return PointerNotRegisteredError(interp, pointer, NULL, TCLH_POINTER_REGISTRATION_MISSING);
}

static Tclh_ReturnCode
PointerVerifyOrUnregisterTagged(Tcl_Interp *interp,
                                Tclh_LibContext *tclhCtxP,
                                const void *pointer,
                                Tclh_PointerTypeTag tag,
                                int unrefCount)
{
    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP == NULL)
        return TCL_ERROR;

    Tcl_HashEntry *he;

    he = Tcl_FindHashEntry(&registryP->pointers, pointer);
    if (he) {
        TclhPointerRecord *ptrRecP = Tcl_GetHashValue(he);
        if (!PointerTypeCompatible(registryP, tag, ptrRecP->tagObj)) {
            return PointerTypeMismatchError(interp, tag, ptrRecP->tagObj);
        }
        if (unrefCount) {
            if (ptrRecP->nRefs == TCLH_POINTER_NREFS_MAX) {
                /* Pinned pointers only affected if ref decrement is MAX */
                if (unrefCount == TCLH_POINTER_NREFS_MAX) {
                    TclhPointerRecordFree(ptrRecP);
                    Tcl_DeleteHashEntry(he);
                }
            } else if (ptrRecP->nRefs <= unrefCount) {
                TclhPointerRecordFree(ptrRecP);
                Tcl_DeleteHashEntry(he);
            } else {
                ptrRecP->nRefs -= unrefCount;
            }
        }
        return TCL_OK;
    }
    if (unrefCount == TCLH_POINTER_NREFS_MAX)
        return TCL_OK; /* Invalidate, no error if pointer not registered */
    else
        return PointerNotRegisteredError(interp, pointer, tag, TCLH_POINTER_REGISTRATION_MISSING);
}

Tclh_ReturnCode
Tclh_PointerUnregisterTagged(Tcl_Interp *interp,
                       Tclh_LibContext *tclhCtxP,
                       const void *pointer,
                       Tclh_PointerTypeTag tag)
{
    return PointerVerifyOrUnregisterTagged(interp, tclhCtxP, pointer, tag, 1);
}

Tclh_ReturnCode
Tclh_PointerInvalidateTagged(Tcl_Interp *interp,
                       Tclh_LibContext *tclhCtxP,
                       const void *pointer,
                       Tclh_PointerTypeTag tag)
{
    return PointerVerifyOrUnregisterTagged(
        interp, tclhCtxP, pointer, tag, TCLH_POINTER_NREFS_MAX);
}

Tcl_Obj *
Tclh_PointerEnumerate(Tcl_Interp *interp,
                      Tclh_LibContext *tclhCtxP,
                      Tclh_PointerTypeTag tag)
{
    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP == NULL) {
            return Tcl_NewObj();
    }

    Tcl_HashEntry *he;
    Tcl_HashSearch hSearch;
    Tcl_HashTable *hTblPtr;
    Tcl_Obj *resultObj = Tcl_NewListObj(0, NULL);

    /* 
     * If tag NULL, match all
     * If tag specified and not "", match the tag
     * If tag is "", match only those without a tag
     */
    int getAll = 0;
    if (tag == NULL)
        getAll = 1;
    else {
        /* Tcl_GetCharLength will shimmer so GetStringFromObj */
        Tcl_Size len;
        (void)Tcl_GetStringFromObj(tag, &len);
        if (len == 0)
            tag = NULL;
    }
    /* Now tag == NULL -> only match records without a tag */
    hTblPtr   = &registryP->pointers;
    for (he = Tcl_FirstHashEntry(hTblPtr, &hSearch);
         he != NULL; he = Tcl_NextHashEntry(&hSearch)) {
        void *pv                   = Tcl_GetHashKey(hTblPtr, he);
        TclhPointerRecord *ptrRecP = Tcl_GetHashValue(he);
        if (getAll || (tag == ptrRecP->tagObj)
            || (tag != NULL
                && PointerTypeMatchesExpected(ptrRecP->tagObj, tag))) {
            Tcl_ListObjAppendElement(
                NULL, resultObj, Tclh_PointerWrap(pv, ptrRecP->tagObj));
        }
    }
    return resultObj;
}

Tclh_ReturnCode
Tclh_PointerVerifyTagged(Tcl_Interp *interp,
                   Tclh_LibContext *tclhCtxP,
                   const void *pointer,
                   Tclh_PointerTypeTag tag)
{
    return PointerVerifyOrUnregisterTagged(interp, tclhCtxP, pointer, tag, 0);
}

Tclh_ReturnCode
Tclh_PointerObjUnregister(Tcl_Interp *interp,
                          Tclh_LibContext *tclhCtxP,
                          Tcl_Obj *objP,
                          void **pointerP,
                          Tclh_PointerTypeTag tag)
{
    void *pv = NULL;            /* Init to keep gcc happy */
    int   tclResult;

    tclResult = Tclh_PointerUnwrapTagged(interp, tclhCtxP, objP, &pv, &tag, tag);
    if (tclResult == TCL_OK) {
        if (pv != NULL)
            tclResult = Tclh_PointerUnregisterTagged(interp, tclhCtxP, pv, tag);
        if (tclResult == TCL_OK && pointerP != NULL)
            *pointerP = pv;
    }
    return tclResult;
}

static Tclh_ReturnCode
PointerObjVerifyOrUnregisterAnyOf(Tcl_Interp *interp,
                                  Tclh_LibContext *tclhCtxP,
                                  Tcl_Obj *objP,
                                  void **pointerP,
                                  int unregister,
                                  va_list args)
{
    if (TclhPointerGetRegistry(interp, tclhCtxP) == NULL)
        return TCL_ERROR;

    int tclResult;
    void *pv = NULL;            /* Init to keep gcc happy */
    Tclh_PointerTypeTag tag = NULL;

    tclResult = TclhUnwrapAnyOfVA(interp, tclhCtxP, objP, &pv, &tag, args);
    if (tclResult == TCL_OK) {
        if (unregister)
            tclResult = Tclh_PointerUnregisterTagged(interp, tclhCtxP, pv, tag);
        else
            tclResult = Tclh_PointerVerifyTagged(interp, tclhCtxP, pv, tag);
        if (tclResult == TCL_OK && pointerP != NULL)
            *pointerP = pv;
    }
    return tclResult;
}

Tclh_ReturnCode
Tclh_PointerObjUnregisterAnyOf(Tcl_Interp *interp,
                               Tclh_LibContext *tclhCtxP,
                               Tcl_Obj *objP,
                               void **pointerP,
                               ... /* tag, ... , NULL */)
{
    int tclResult;
    va_list args;

    va_start(args, pointerP);
    tclResult = PointerObjVerifyOrUnregisterAnyOf(interp, tclhCtxP, objP, pointerP, 1, args);
    va_end(args);
    return tclResult;
}

Tclh_ReturnCode
Tclh_PointerObjVerify(Tcl_Interp *interp,
                      Tclh_LibContext *tclhCtxP,
                      Tcl_Obj *objP,
                      void **pointerP,
                      Tclh_PointerTypeTag *tagP,
                      Tclh_PointerTypeTag expectedTag)
{
    void *pv = NULL;            /* Init to keep gcc happy */
    int   tclResult;
    Tclh_PointerTypeTag tag;

    tclResult = Tclh_PointerUnwrapTagged(interp, tclhCtxP, objP, &pv, &tag, expectedTag);
    if (tclResult == TCL_OK) {
        if (pv == NULL)
            tclResult = Tclh_ErrorPointerNull(interp);
        else {
            tclResult = Tclh_PointerVerifyTagged(interp, tclhCtxP, pv, tag);
            if (tclResult == TCL_OK) {
                if (pointerP)
                    *pointerP = pv;
                if (tagP)
                    *tagP = tag;
            }
        }
    }
    return tclResult;
}

Tclh_ReturnCode
Tclh_PointerObjVerifyAnyOf(Tcl_Interp *interp,
                           Tclh_LibContext *tclhCtxP,
                           Tcl_Obj *objP,
                           void **pointerP,
                           ... /* tag, tag, NULL */)
{
    int tclResult;
    va_list args;

    va_start(args, pointerP);
    tclResult = PointerObjVerifyOrUnregisterAnyOf(
        interp, tclhCtxP, objP, pointerP, 0, args);
    va_end(args);
    return tclResult;
}

Tclh_ReturnCode
Tclh_PointerSubtagDefine(Tcl_Interp *interp,
                         Tclh_LibContext *tclhCtxP,
                         Tclh_PointerTypeTag subtagObj,
                         Tclh_PointerTypeTag supertagObj)
{
    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP == NULL)
        return TCL_ERROR;

    int tclResult;
    const char *subtag;

    if (supertagObj == NULL)
        return TCL_OK; /* void* always a supertype, no need to register */
    subtag = Tcl_GetString(subtagObj);
    if (!strcmp(subtag, Tcl_GetString(supertagObj)))
        return TCL_OK; /* Same tag */
    tclResult = Tclh_HashAdd(
        interp, &registryP->castables, subtag, supertagObj);
    if (tclResult == TCL_OK)
        Tcl_IncrRefCount(supertagObj);/* Since added to hash table */
    return tclResult;
}

Tclh_ReturnCode
Tclh_PointerSubtagRemove(Tcl_Interp *interp,
                         Tclh_LibContext *tclhCtxP,
                         Tclh_PointerTypeTag tagObj)
{
    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP == NULL)
        return TCL_ERROR;

    Tcl_HashEntry *he;

    if (tagObj) {
        he = Tcl_FindHashEntry(&registryP->castables, Tcl_GetString(tagObj));
        if (he) {
            Tcl_Obj *objP = Tcl_GetHashValue(he);
            if (objP)
                Tcl_DecrRefCount(objP);
            Tcl_DeleteHashEntry(he);
        }
    }
    return TCL_OK;
}

Tcl_Obj *
Tclh_PointerSubtags(Tcl_Interp *interp, Tclh_LibContext *tclhCtxP)
{
    Tcl_Obj *objP;

    objP = Tcl_NewListObj(0, NULL);
    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP == NULL)
        return objP;

    Tcl_HashEntry *heP;
    Tcl_HashTable *htP;
    Tcl_HashSearch hSearch;

    htP  = &registryP->castables;

    for (heP = Tcl_FirstHashEntry(htP, &hSearch); heP != NULL;
         heP = Tcl_NextHashEntry(&hSearch)) {
        Tcl_Obj *subtagObj = Tcl_NewStringObj(Tcl_GetHashKey(htP, heP), -1);
        Tcl_Obj *supertagObj = Tcl_GetHashValue(heP);
        if (supertagObj == NULL)
            supertagObj = Tcl_NewObj();
        Tcl_ListObjAppendElement(NULL, objP, subtagObj);
        Tcl_ListObjAppendElement(NULL, objP, supertagObj);
    }

    return objP;
}

Tclh_ReturnCode
Tclh_PointerCast(Tcl_Interp *interp,
                 Tclh_LibContext *tclhCtxP,
                 Tcl_Obj *objP,
                 Tclh_PointerTypeTag newTag,
                 Tcl_Obj **castPtrObj)
{
    Tclh_PointerTypeTag oldTag;
    void *pv;
    int tclResult;

    /* First extract pointer value and tag */
    tclResult = Tclh_PointerObjGetTag(interp, objP, &oldTag);
    if (tclResult != TCL_OK)
        return tclResult;

    /* TBD - why not just call Tclh_PointerUnwrap here? */
    tclResult = Tclh_PointerUnwrapTagged(interp, tclhCtxP, objP, &pv, NULL, NULL);
    if (tclResult != TCL_OK)
        return tclResult;

    /* Validate that if registered, it is registered correctly. */
    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP) {
        TclhPointerRecord *ptrRecP = NULL;
        Tcl_HashEntry *he = Tcl_FindHashEntry(&registryP->pointers, pv);
        if (he) {
            ptrRecP = Tcl_GetHashValue(he);
            if (!PointerTypeCompatible(registryP, oldTag, ptrRecP->tagObj)
                && !PointerTypeCompatible(registryP, ptrRecP->tagObj, oldTag)) {
                return PointerTypeMismatchError(interp, oldTag, ptrRecP->tagObj);
            }
        }
        /* Must be either upcastable or downcastable */
        if (!PointerTypeCompatible(registryP, oldTag, newTag)
            && !PointerTypeCompatible(registryP, newTag, oldTag)) {
            return PointerTypeMismatchError(interp, oldTag, newTag);
        }
        /* If registered, we have to change registration unless pinned */
        if (ptrRecP && ptrRecP->nRefs != TCLH_POINTER_NREFS_MAX) {
            Tclh_PointerTypeTag tempTag;
            tempTag         = ptrRecP->tagObj;
            ptrRecP->tagObj = newTag;
            if (newTag)
                Tcl_IncrRefCount(newTag);
            if (tempTag)
                Tcl_DecrRefCount(tempTag); /* AFTER incr-ing newTag */
        }
    }

    *castPtrObj = Tclh_PointerWrap(pv, newTag);
    return TCL_OK;
}

static int
PointerTagsSame(Tclh_PointerTypeTag tagA, Tclh_PointerTypeTag tagB)
{
    if (tagA == tagB)
        return 1;
    if (tagA == NULL || tagB == NULL)
        return 0;
    return !strcmp(Tcl_GetString(tagA), Tcl_GetString(tagB));
}

static int
PointerTagIsAncestor(TclhPointerRegistry *registryP,
                             Tclh_PointerTypeTag tag,
                             Tclh_PointerTypeTag ancestor)
{
    if (ancestor == NULL)
        return 1; /* ancestor is void*. All tags can be implicitly cast to it */
    if (tag == NULL)
        return 0;

    /*
     * Recursively look for an ancestor. Rather than trying to detect loops
     * by recording history just keep a hard limit 10 on the depth of lookups
     */
    int i;
    for (i = 0; i < 10; ++i) {
        Tcl_HashEntry *he;
        he = Tcl_FindHashEntry(&registryP->castables, Tcl_GetString(tag));
        if (he == NULL)
            return 0;/* No super type */
        tag = Tcl_GetHashValue(he);
        /* For NULL, if first did not match succeeding will not either cut short */
        if (tag == NULL)
            return 0;
        if (PointerTagsSame(tag, ancestor))
            return 1;
    }

    return 0;

}

static Tclh_PointerTagRelation
PointerTagCompare(TclhPointerRegistry *registryP,
                  Tclh_PointerTypeTag tag,
                  Tclh_PointerTypeTag expectedTag)
{
    if (tag == expectedTag)
        return TCLH_TAG_RELATION_EQUAL;
    if (expectedTag == NULL)
        return TCLH_TAG_RELATION_IMPLICITLY_CASTABLE;
    if (tag == NULL)
        return TCLH_TAG_RELATION_EXPLICITLY_CASTABLE;
    if (!strcmp(Tcl_GetString(tag), Tcl_GetString(expectedTag)))
        return TCLH_TAG_RELATION_EQUAL;
    if (PointerTagIsAncestor(registryP, tag, expectedTag))
        return TCLH_TAG_RELATION_IMPLICITLY_CASTABLE;
    if (PointerTagIsAncestor(registryP, expectedTag, tag))
        return TCLH_TAG_RELATION_EXPLICITLY_CASTABLE;
    return TCLH_TAG_RELATION_UNRELATED;
}

static Tclh_PointerRegistrationStatus
PointerRegistrationStatus(TclhPointerRegistry *registryP,
                          void *pv,
                          Tclh_PointerTypeTag tag)
{
    Tcl_HashEntry *he;

    he = Tcl_FindHashEntry(&registryP->pointers, pv);
    if (he == NULL) {
        return TCLH_POINTER_REGISTRATION_MISSING;
    }
    else {
        TclhPointerRecord *ptrRecP = Tcl_GetHashValue(he);
        switch (PointerTagCompare(registryP, tag, ptrRecP->tagObj)) {
        case TCLH_TAG_RELATION_EQUAL:
            return TCLH_POINTER_REGISTRATION_OK;
        case TCLH_TAG_RELATION_IMPLICITLY_CASTABLE:
            return TCLH_POINTER_REGISTRATION_DERIVED;
        default:
            return TCLH_POINTER_REGISTRATION_WRONGTAG;
        }
    }
}

int
Tclh_PointerRegistered(Tcl_Interp *interp, Tclh_LibContext *tclhCtxP, const void *pv)
{
    if (pv == NULL)
        return 0;
    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP == NULL)
        return 0;
    Tcl_HashEntry *he;
    he = Tcl_FindHashEntry(&registryP->pointers, pv);
    return he != NULL;
}

Tclh_ReturnCode
Tclh_PointerVerify(Tcl_Interp *interp, Tclh_LibContext *tclhCtxP, const void *pv)
{
    if (pv == NULL)
        return Tclh_ErrorPointerNull(interp);
    if (Tclh_PointerRegistered(interp, tclhCtxP, pv))
        return TCL_OK;
    return PointerNotRegisteredError(interp, pv, NULL, TCLH_POINTER_REGISTRATION_MISSING);
}

Tclh_ReturnCode
Tclh_PointerObjDissect(Tcl_Interp *interp,
                       Tclh_LibContext *tclhCtxP,
                       Tcl_Obj *ptrObj,
                       Tclh_PointerTypeTag expectedTag,
                       void **pvP,
                       Tclh_PointerTypeTag *ptrTagP,
                       Tclh_PointerTagRelation *tagMatchP,
                       Tclh_PointerRegistrationStatus *registeredP)
{
    Tclh_PointerTypeTag tag;
    void *pv;

    /* Try converting Tcl_Obj internal rep */
    if (ptrObj->typePtr != &gPointerType) {
        if (SetPointerFromAny(interp, ptrObj) != TCL_OK)
            return TCL_ERROR;
    }

    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP == NULL)
        return TCL_ERROR;

    tag = PointerTypeGet(ptrObj);
    pv  = PointerValueGet(ptrObj);

    if (pvP)
        *pvP = pv;

    if (ptrTagP)
        *ptrTagP = tag;

    if (tagMatchP)
        *tagMatchP = PointerTagCompare(registryP, tag, expectedTag);

    if (registeredP)
        *registeredP = PointerRegistrationStatus(registryP, pv, tag);

    return TCL_OK;
}

Tcl_Obj *
Tclh_PointerObjInfo(Tcl_Interp *interp,
                    Tclh_LibContext *tclhCtxP,
                    Tcl_Obj *ptrObj)
{
    Tclh_PointerTypeTag ptrTag;
    void *pv;

    if (ptrObj->typePtr != &gPointerType) {
        if (SetPointerFromAny(interp, ptrObj) != TCL_OK)
            return NULL;
    }

    TclhPointerRegistry *registryP = TclhPointerGetRegistry(interp, tclhCtxP);
    if (registryP == NULL)
        return NULL;

    Tcl_Obj *infoObjs[10];
    int nInfoObjs;

    infoObjs[0] = Tcl_NewStringObj("Tag", 3);
    ptrTag      = PointerTypeGet(ptrObj);
    infoObjs[1] = ptrTag ? ptrTag : Tcl_NewObj();

    pv = PointerValueGet(ptrObj);

    Tcl_HashEntry *he;

    infoObjs[2] = Tcl_NewStringObj("Registration", 12);
    he = Tcl_FindHashEntry(&registryP->pointers, pv);
    if (he == NULL) {
        infoObjs[3] = Tcl_NewStringObj("none", 4);
        nInfoObjs   = 4;
    }
    else {
        TclhPointerRecord *ptrRecP = Tcl_GetHashValue(he);
        if (ptrRecP->nRefs < 0)
            infoObjs[3] = Tcl_NewStringObj("safe", 4);
        else if (ptrRecP->nRefs == TCLH_POINTER_NREFS_MAX)
            infoObjs[3] = Tcl_NewStringObj("pinned", 6);
        else
            infoObjs[3] = Tcl_NewStringObj("counted", 7);

        infoObjs[4] = Tcl_NewStringObj("Match", 5);
        switch (PointerTagCompare(registryP, ptrTag, ptrRecP->tagObj)) {
        case TCLH_TAG_RELATION_EQUAL:
            infoObjs[5] = Tcl_NewStringObj("exact", 5);
            break;
        case TCLH_TAG_RELATION_IMPLICITLY_CASTABLE:
            infoObjs[5] = Tcl_NewStringObj("derived", 7);
            break;
        default:
            infoObjs[5] = Tcl_NewStringObj("mismatch", 8);
            break;
        }
        infoObjs[6] = Tcl_NewStringObj("RegisteredTag", 13);
        infoObjs[7] = ptrRecP->tagObj ? ptrRecP->tagObj : Tcl_NewObj();
        nInfoObjs   = 8;
    }

    return Tcl_NewListObj(nInfoObjs, infoObjs);
}