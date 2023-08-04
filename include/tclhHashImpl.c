/*
 * Copyright (c) 2022-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhHash.h"

Tclh_ReturnCode
Tclh_HashAdd(Tcl_Interp *ip,
             Tcl_HashTable *htP,
             const void *key,
             ClientData value)
{
    Tcl_HashEntry *heP;
    int isNew;

    heP = Tcl_CreateHashEntry(htP, key, &isNew);
    if (!isNew)
        return Tclh_ErrorExists(ip, "Name", NULL, NULL);
    Tcl_SetHashValue(heP, value);
    return TCL_OK;
}

Tclh_Bool
Tclh_HashAddOrReplace(Tcl_HashTable *htP,
                      const void *key,
                      ClientData value,
                      ClientData *oldValueP)
{
    Tcl_HashEntry *heP;
    int isNew;

    heP = Tcl_CreateHashEntry(htP, key, &isNew);
    if (!isNew && oldValueP)
        *oldValueP = Tcl_GetHashValue(heP);
    Tcl_SetHashValue(heP, value);
    return isNew;
}

Tclh_ReturnCode
Tclh_HashLookup(Tcl_HashTable *htP, const void *key, ClientData *valueP)
{
    Tcl_HashEntry *heP;
    heP = Tcl_FindHashEntry(htP, key);
    if (heP) {
        if (valueP)
            *valueP = Tcl_GetHashValue(heP);
        return TCL_OK;
    }
    else
        return TCL_ERROR;
}

Tclh_Bool
Tclh_HashIterate(Tcl_HashTable *htP,
                 int (*fnP)(Tcl_HashTable *, Tcl_HashEntry *, ClientData),
                 ClientData fnData)
{
    Tcl_HashEntry *heP;
    Tcl_HashSearch hSearch;

    for (heP = Tcl_FirstHashEntry(htP, &hSearch); heP != NULL;
         heP = Tcl_NextHashEntry(&hSearch)) {
        if (fnP(htP, heP, fnData) == 0)
            return 0;
    }
    return 1;
}