#ifndef TCLHATOM_H
#define TCLHATOM_H

/*
 * Copyright (c) 2021-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhBase.h"

/* Typedef: Tclh_AtomRegistry
 *
 * See <Atoms>.
 */
typedef Tcl_HashTable *Tclh_AtomRegistry;

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
 * interp - Tcl interpreter in which to initialize.
 * atomRegistryP - Location to store the atom table. May be NULL.
 *
 * Any allocated resource are automatically freed up when the interpreter
 * is deleted.
 *
 * Returns:
 * TCL_OK    - Library was successfully initialized.
 * TCL_ERROR - Initialization failed. Library functions must not be called.
 *             An error message is left in the interpreter result.
 */
Tclh_ReturnCode Tclh_AtomLibInit(Tcl_Interp *interp,
                                 Tclh_AtomRegistry *ptrRegP);

/* Function: Tclh_GetAtom
 * Returns a Tcl_Obj wrapping the string value.
 *
 * Parameters:
 * interp  - Tcl interpreter in which the atom is to be registered.
 *           May be NULL.
 * registryP - pointer registry. If NULL, the registry associated with the
 *             interpreter is used.
 * str - the string value to wrap
 *
 * The returned Tcl_Obj will have a reference held to it by the registry.
 * Caller must not call Tcl_DecrRefCount on it without doing a Tcl_IncrRefCount
 * itself. Conversely, to prevent the returned Tcl_Obj from going away if
 * the registry is purged, calle should call Tcl_IncrRefCount on it if it
 * intends to hold on to it. Finally, the Tcl_Obj may be shared so caller should
 * follow the usual rules for a shared object.
 *
 * Returns:
 * Pointer to a Tcl_Obj containing the value. The function will panic on memory
 * allocation failure.
 */
Tcl_Obj *Tclh_AtomGet(Tcl_Interp *interp, Tclh_AtomRegistry registry, const char *str);

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

typedef Tcl_HashTable TclhAtomRegistryInfo;
static TclhAtomRegistryInfo *TclhInitAtomRegistry(Tcl_Interp *interp);

Tclh_ReturnCode
Tclh_AtomLibInit(Tcl_Interp *interp, Tclh_AtomRegistry *ptrRegP)
{
    int ret;
    ret = Tclh_BaseLibInit(interp);
    if (ret == TCL_OK) {
        Tclh_AtomRegistry reg;
        reg = TclhInitAtomRegistry(interp);
        if (reg) {
            if (ptrRegP)
                *ptrRegP = reg;
        }
        else
            ret = TCL_ERROR;
    }
    return ret;
}

Tcl_Obj *Tclh_AtomGet(Tcl_Interp *interp, TclhAtomRegistryInfo *registryP, const char *str)
{
    if (registryP == NULL) {
        if (interp == NULL) {
            return NULL; /* Either interp or registryP should have been
                                 input */
        }
        registryP = TclhInitAtomRegistry(interp);
    }
    Tcl_HashEntry *he;
    int new_entry;
    he = Tcl_CreateHashEntry(registryP, str, &new_entry);
    if (new_entry) {
        Tcl_Obj *objP = Tcl_NewStringObj(str, -1);
        Tcl_IncrRefCount(objP);
        Tcl_SetHashValue(he, objP);
        return objP;
    } else {
        return (Tcl_Obj *) Tcl_GetHashValue(he);
    }
}

static void
TclhCleanupAtomRegistry(ClientData clientData, Tcl_Interp *interp)
{
    Tcl_HashTable *registryP = (TclhAtomRegistryInfo *)clientData;
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

#ifndef TCLH_ATOM_REGISTRY_NAME
/* This will be shared for all extensions if embedder has not defined it */
# define TCLH_ATOM_REGISTRY_NAME "TclhAtomTable"
#endif

static TclhAtomRegistryInfo *
TclhInitAtomRegistry(Tcl_Interp *interp)
{
    TclhAtomRegistryInfo *registryP;
    const char * const atomTableKey = TCLH_ATOM_REGISTRY_NAME;
    registryP = Tcl_GetAssocData(interp, atomTableKey, NULL);
    if (registryP == NULL) {
        registryP = (TclhAtomRegistryInfo *) Tcl_Alloc(sizeof(*registryP));
        Tcl_InitHashTable(registryP, TCL_STRING_KEYS);
        Tcl_SetAssocData(
            interp, atomTableKey, TclhCleanupAtomRegistry, registryP);
    }
    return registryP;
}

#endif /* TCLH_ATOM_IMPL */
#endif
