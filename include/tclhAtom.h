#ifndef TCLHATOM_H
#define TCLHATOM_H

/*
 * Copyright (c) 2021-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhBase.h"

/* Section: Atoms
 *
 * Provides a facility to allow frequently used string values to be shared by
 * allocation of a single Tcl_Obj.
 *
 */

/* Function: Tclh_AtomLibInit
 * Must be called to initialize the Atom module before any of
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
 * Any allocated resources are automatically freed up when the interpreter
 * is deleted.
 *
 * Returns:
 * TCL_OK    - Library was successfully initialized.
 * TCL_ERROR - Initialization failed. Library functions must not be called.
 *             An error message is left in the interpreter result.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_AtomLibInit(Tcl_Interp *interp,
                                 Tclh_LibContext *tclhCtxP);

/* Function: Tclh_GetAtom
 * Returns a Tcl_Obj wrapping the string value.
 *
 * Parameters:
 * interp - Tcl interpreter for error messages. May be NULL.
 * tclhCtxP - Tclh context as returned by <Tclh_LibInit> to use. If NULL,
 *    the Tclh context associated with the interpreter is used after
 *    initialization if necessary.
 * str - the string value to atomize.
 *
 * At least one of interp and tclhCtxP must be non-NULL.
 *
 * The returned Tcl_Obj will have a reference held to it by the registry.
 * Caller must not call Tcl_DecrRefCount on it without doing a Tcl_IncrRefCount
 * itself. Conversely, to prevent the returned Tcl_Obj from going away if
 * the registry is purged, caller should call Tcl_IncrRefCount on it if it
 * intends to hold on to it. Finally, the Tcl_Obj may be shared so caller should
 * follow the usual rules for a shared object.
 *
 * Returns:
 * Pointer to a Tcl_Obj containing the value. The function will panic on memory
 * allocation failure.
 */
TCLH_LOCAL Tcl_Obj *
Tclh_AtomGet(Tcl_Interp *interp, Tclh_LibContext *ctx, const char *str);

#ifdef TCLH_SHORTNAMES

#define AtomLibInit Tclh_AtomLibInit
#define AtomGet     Tclh_AtomGet

#endif

/*
 * IMPLEMENTATION
 */

#ifdef TCLH_IMPL
# define TCLH_ATOM_IMPL
#endif

#ifdef TCLH_ATOM_IMPL

static void
TclhCleanupAtomRegistry(ClientData clientData, Tcl_Interp *interp)
{
    Tcl_HashTable *registryP = (Tcl_HashTable *)clientData;
    TCLH_ASSERT(registryP);

    Tcl_HashEntry *he;
    Tcl_HashSearch hSearch;

    for (he = Tcl_FirstHashEntry(registryP, &hSearch); he != NULL;
         he = Tcl_NextHashEntry(&hSearch)) {
        Tcl_Obj *objP = (Tcl_Obj *)Tcl_GetHashValue(he);
        Tcl_DecrRefCount(objP);
    }
    Tcl_DeleteHashTable(registryP);
    Tcl_Free((void *)registryP);
}

Tclh_ReturnCode
Tclh_AtomLibInit(Tcl_Interp *interp, Tclh_LibContext *tclhCtxP)
{
    Tclh_ReturnCode ret;

    if (tclhCtxP == NULL) {
        if (interp == NULL)
            return TCL_ERROR;
        ret = Tclh_LibInit(interp, &tclhCtxP);
        if (ret != TCL_OK)
            return ret;
    }

    if (tclhCtxP->atomRegistryP)
        return TCL_OK; /* Already done */

    Tcl_HashTable *htP =
        (Tcl_HashTable *)Tcl_Alloc(sizeof(*tclhCtxP->atomRegistryP));
    Tcl_InitHashTable(htP, TCL_STRING_KEYS);
    Tcl_CallWhenDeleted(interp, TclhCleanupAtomRegistry, htP);
    tclhCtxP->atomRegistryP = htP;

    return TCL_OK;
}

Tcl_Obj *
Tclh_AtomGet(Tcl_Interp *interp, Tclh_LibContext *tclhCtxP, const char *str)
{
    Tclh_ReturnCode ret;
    if (tclhCtxP == NULL) {
        if (interp == NULL)
            return NULL;
        ret = Tclh_LibInit(interp, &tclhCtxP);
        if (ret != TCL_OK)
            return NULL;
    }
    if (tclhCtxP->atomRegistryP == NULL) {
        Tclh_ErrorGeneric(
            interp, NULL, "Internal error: Tclh context not initialized.");
        return NULL;
    }
    Tcl_HashTable *htP = tclhCtxP->atomRegistryP;
    Tcl_HashEntry *he;
    int new_entry;
    he = Tcl_CreateHashEntry(htP, str, &new_entry);
    if (new_entry) {
        Tcl_Obj *objP = Tcl_NewStringObj(str, -1);
        Tcl_IncrRefCount(objP);
        Tcl_SetHashValue(he, objP);
        return objP;
    } else {
        return (Tcl_Obj *) Tcl_GetHashValue(he);
    }
}

#endif /* TCLH_ATOM_IMPL */
#endif
