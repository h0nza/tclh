/*
 * Copyright (c) 2021-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhAtom.h"

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