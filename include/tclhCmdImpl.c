/*
 * Copyright (c) 2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhCmd.h"

Tclh_ReturnCode
Tclh_SubCommandNameToIndex(Tcl_Interp *ip,
                           Tcl_Obj *nameObj,
                           Tclh_SubCommand *cmdTableP,
                           int *indexP)
{
    return Tcl_GetIndexFromObjStruct(
        ip, nameObj, cmdTableP, sizeof(*cmdTableP), "subcommand", 0, indexP);
}


Tclh_ReturnCode
Tclh_SubCommandLookup(Tcl_Interp *ip,
                      const Tclh_SubCommand *cmdTableP,
                      int objc,
                      Tcl_Obj *const objv[],
                      int *indexP)
{
    if (objc < 2) {
        return Tclh_ErrorNumArgs(ip, 1, objv, "subcommand ?arg ...?");
    }
    if (Tcl_GetIndexFromObjStruct(
            ip, objv[1], cmdTableP, sizeof(*cmdTableP), "subcommand", 0, indexP)
        != TCL_OK)
        return TCL_ERROR;

    cmdTableP += *indexP;
    /*
     * Can't use CHECK_NARGS here because of slightly different semantics.
     */
    if ((objc-2) < cmdTableP->minargs || (objc-2) > cmdTableP->maxargs) {
        return Tclh_ErrorNumArgs(ip, 2, objv, cmdTableP->message);
    }
    return TCL_OK;
}