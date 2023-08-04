#ifndef TCLHCMD_H
#define TCLHCMD_H

/*
 * Copyright (c) 2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhBase.h"

/* Section: Command implementation utilities
 * 
 * Contains functions commonly needed when defining Tcl commands.
 */

/* Typedef: Tclh_SubCommand
 *
 * Defines the descriptor for a single subcommand in a command ensemble.
 */
typedef struct Tclh_SubCommand {
    const char *cmdName;
    int minargs;
    int maxargs;
    const char *message;
    int (*cmdFn)();
    int flags; /* Command specific usage */
} Tclh_SubCommand;

/* Function: Tclh_CmdLibInit
 * Must be called to initialize the Cmd module before any of
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
 * TCL_ERROR - Initialization failed. module functions must not be called.
 *             An error message is left in the interpreter result.
 */
TCLH_INLINE Tclh_ReturnCode
Tclh_CmdLibInit(Tcl_Interp *ip, Tclh_LibContext *tclhCtxP)
{
    if (tclhCtxP == NULL)
        return Tclh_LibInit(ip, NULL);
    return TCL_OK;
}

/* Function: Tclh_SubCommandNameToIndex
 * Looks up a subcommand table and returns index of a matching entry.
 *
 * The function looks up the subcommand table to find the entry with
 * name given by *nameObj*. Each entry in the table is a <Tclh_SubCommand>
 * structure and the name is matched against the *cmdName* field of that
 * structure. Unique prefixes of subcommands are accepted as a match.
 * The end of the table is indicated by an entry whose *cmdName* field is NULL.
 *
 * This function only matches against the name and does not concern itself
 * with any of the other fields in an entry.
 *
 * The function is a wrapper around the Tcl *Tcl_GetIndexFromObjStruct*
 * and subject to its constraints. In particular, *cmdTableP* must point
 * to static storage as a pointer to it is retained.
 *
 * Parameters:
 * ip - Interpreter
 * nameObj - name of method to lookup
 * cmdTableP - pointer to command table
 * indexP - location to store index of matched entry
 *
 * Returns:
 * *TCL_OK* on success with index of match stored in *indexP*; otherwise,
 * *TCL_ERROR* with error message in *ip*.
 */
TCLH_LOCAL Tclh_ReturnCode Tclh_SubCommandNameToIndex(
    Tcl_Interp *ip, Tcl_Obj *nameObj, Tclh_SubCommand *cmdTableP, int *indexP);

/* Function: Tclh_SubCommandLookup
 * Looks up a subcommand table and returns index of a matching entry after
 * verifying number of arguments.
 *
 * The function looks up the subcommand table to find the entry with
 * name given by *objv[1]*. Each entry in the table is a <Tclh_SubCommand>
 * structure and the name is matched against the *cmdName* field of that
 * structure. Unique prefixes of subcommands are accepted as a match.
 * The end of the table is indicated by an entry whose *cmdName* field is NULL.
 *
 * This function only matches against the name and does not concern itself
 * with any of the other fields in an entry.
 *
 * The function is a wrapper around the Tcl *Tcl_GetIndexFromObjStruct*
 * and subject to its constraints. In particular, *cmdTableP* must point
 * to static storage as a pointer to it is retained.
 *
 * On finding a match, the function verifies that the number of arguments
 * satisfy the number of arguments expected by the subcommand based on
 * the *minargs* and *maxargs* fields of the matching <Tclh_SubCommand>
 * entry. These fields should hold the argument count range for the
 * subcommand (meaning not counting subcommand itself)
 *
 * Parameters:
 * ip - Interpreter
 * cmdTableP - pointer to command table
 * objc - Number of elements in *objv*. Must be at least *1*.
 * objv - Array of *Tcl_Obj* pointers as passed to Tcl commands. *objv[0]*
 *        is expected to be the main command and *objv[1]* the subcommand.
 * indexP - location to store index of matched entry
 *
 * Returns:
 * *TCL_OK* on success with index of match stored in *indexP*; otherwise,
 * *TCL_ERROR* with error message in *ip*.
 */
TCLH_LOCAL Tclh_ReturnCode
Tclh_SubCommandLookup(Tcl_Interp *ip,
                      const Tclh_SubCommand *cmdTableP,
                      int objc,
                      Tcl_Obj *const objv[],
                      int *indexP);


#ifdef TCLH_SHORTNAMES
#define SubCommandNameToIndex Tclh_SubCommandNameToIndex
#define SubCommandLookup      Tclh_SubCommandLookup
#endif

#ifdef TCLH_IMPL
#include "tclhCmdImpl.c"
#endif

#endif