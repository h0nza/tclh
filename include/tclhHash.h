#ifndef TCLHHASH_H
#define TCLHHASH_H

/*
 * Copyright (c) 2022-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhBase.h"

/* Section: Hash table utilities
 *
 * Contains utilities dealing with Tcl hash tables.
 */

/* Function: Tclh_HashLibInit
 * Must be called to initialize the Hash module before any of
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
TCLH_INLINE Tclh_ReturnCode
Tclh_HashLibInit(Tcl_Interp *ip, Tclh_LibContext *tclhCtxP)
{
    if (tclhCtxP == NULL)
        return Tclh_LibInit(ip, NULL);
    return TCL_OK;
}


/* Function: Tclh_HashAdd
 * Adds an entry to a table of names
 *
 * Parameters:
 * ip - interpreter. May be NULL if error messages not desired.
 * htP - table of names
 * key - key to add
 * value - value to associate with the name
 *
 * An error is returned if the entry already exists.
 *
 * Returns:
 * TCL_OK on success, TCL_ERROR on failure.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_HashAdd(Tcl_Interp *ip,
                             Tcl_HashTable *htP,
                             const void *key,
                             ClientData value);

/* Function: Tclh_HashAddOrReplace
 * Adds or replaces an entry to a table of names
 *
 * Parameters:
 * htP - table of names
 * key - key to add
 * value - value to associate with the name
 * oldValueP - location to store old value if an existing entry was replaced.
 *    May be *NULL*.
 *
 * If the function returns 0, an existing entry was replaced, and
 * the value previous associated with the name is returned in *oldValueP*.
 *
 * Returns:
 * 0 if the name already existed, and non-0 if a new entry was added.
 */
TCLH_LOCAL Tclh_Bool Tclh_HashAddOrReplace(Tcl_HashTable *htP,
                                const void *key,
                                ClientData value,
                                ClientData *oldValueP);

/* Function: Tclh_HashIterate
 * Invokes a specified function on all entries in a hash table
 *
 * Parameters:
 * htP - hash table
 * fnP - function to call for each entry
 * fnData - any value to pass to fnP
 *
 * The callback function *fnP* is passed a pointer to the hash table, hash
 * entry and an opaque client value. It should follow the rules for Tcl hash
 * tables and not modify the table itself except for optionally deleting the
 * passed entry. The iteration will terminate if the callback function
 * returns 0.
 *
 * Returns:
 * Returns *1* if the iteration terminated normally with all entries processed,
 * and *0* if it was terminated by the callback returning *0*.
 */
TCLH_LOCAL Tclh_Bool
Tclh_HashIterate(Tcl_HashTable *htP,
                 int (*fnP)(Tcl_HashTable *, Tcl_HashEntry *, ClientData),
                 ClientData fnData);

/* Function: Tclh_HashLookup
 * Retrieves the value associated with a key in a hash table.
 *
 * Parameters:
 * htP - the hash table
 * key - key to look up
 * valueP - location to store the value. May be NULL if only existence
 *    is being checked.
 *
 * Returns:
 * *TCL_OK* if found, *TCL_ERROR* on failure.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_HashLookup(Tcl_HashTable *htP, const void *key, ClientData *valueP);

/* Function: Tclh_HashRemove
 * Removes an entry from a hash table, returning its value.
 *
 * Parameters:
 * htP - hash table
 * key - key to remove
 * valueP - location to store the value. May be NULL if not of interest.
 *
 * Returns:
 * *TCL_OK* if key existed in table, *TCL_ERROR* otherwise.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_HashRemove(Tcl_HashTable *htP, const void *key, ClientData *valueP);

#ifdef TCLH_SHORTNAMES
#define HashAdd          Tclh_HashAdd
#define HashAddOrReplace Tclh_HashAddOrReplace
#define HashIterate      Tclh_HashIterate
#define HashLookup       Tclh_HashLookup
#define HashRemove       Tclh_HashRemove
#endif

#ifdef TCLH_IMPL
#include "tclhHashImpl.c"
#endif

#endif /* TCLHHASH_H */