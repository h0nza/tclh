#ifndef TCLHWRAP_H
#define TCLHWRAP_H

/*
 * Copyright (c) 2021-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tcl.h"
#include "tclhBase.h"

#ifdef _WIN32
# include <rpc.h>
  typedef UUID Tclh_UUID;
#else
# include <uuid/uuid.h>
  typedef struct Tclh_UUID {
      uuid_t bytes;
  } Tclh_UUID;
#endif

/* Function: Tclh_UuidNewObj
 * Generate a new UUID. The UUID is not guaranteed to be cryptographically
 * secure.
 *
 * Returns:
 * Pointer to a Tcl_Obj wrapping a generated UUID.
 */
Tcl_Obj *Tclh_UuidNewObj ();

/* Function: Tclh_UuidWrap
 * Wraps a Tclh_UUID as a Tcl_Obj.
 *
 * Parameters:
 * uuidP - pointer to a binary UUID.
 *
 * Returns:
 * The Tcl_Obj containing the wrapped pointer.
 */
Tcl_Obj *Tclh_UuidWrap (const Tclh_UUID *uuidP);

/* Function: Tclh_UuidUnwrap
 * Unwraps a Tcl_Obj containing a UUID.
 *
 * Parameters:
 * interp - Pointer to interpreter
 * objP   - pointer to the Tcl_Obj wrapping the UUID.
 * uuidP  - Location to store UUID.
 *
 * Returns:
 * TCL_OK    - Success, UUID stored in *uuidP.
 * TCL_ERROR - Failure, error message stored in interp.
 */
Tclh_ReturnCode
Tclh_UuidUnwrap(Tcl_Interp *interp, Tcl_Obj *objP, Tclh_UUID *uuidP);

/* Function: Tclh_UuidIsObjIntrep
 * Checks if the passed Tcl_Obj currently holds an internal representation
 * of a UUID. This function's purpose is primarily as an optimization to
 * avoid unnecessary string generation and shimmering when a Tcl_Obj could
 * be one of several types.
 * For example, suppose an argument could be either a integer or a Uuid.
 * Checking for an integer via Tcl_GetIntFromObj would cause generation"
 * of a string from the Uuid unnecesarily. Instead the caller can call
 * Tclh_UuidIsObjIntrep and if it returns 1, not even bother to check
 * integer value. Obviously this only works if the string representation
 * of Uuid cannot be interpreter as the other type.
 * 
 * Parameters:
 * objP - the Tcl_Obj to be checked.
 *
 * Returns:
 * 1 - Current internal representation holds a Uuid.
 * 0 - otherwise.
 */
Tclh_Bool Tclh_UuidIsObjIntrep (Tcl_Obj *objP);

#ifdef TCLH_SHORTNAMES
#define UuidNewObj Tclh_UuidNewObj
#define UuidWrap   Tclh_UuidWrap
#define UuidUnwrap Tclh_UuidUnwrap
#define UuidIsObjIntrep tclh_UuidIsObjIntrep
#endif

#ifdef TCLH_IMPL
#include "tclhUuidImpl.c"
#endif

#endif /* TCLHWRAP_H */
