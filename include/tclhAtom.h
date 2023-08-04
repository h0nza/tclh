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

#ifdef TCLH_IMPL
#include "tclhAtomImpl.c"
#endif

#endif
