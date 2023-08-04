/*
 * Copyright (c) 2022, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include "tclhNamespace.h"

int Tclh_NsLibInit(Tcl_Interp *ip, Tclh_LibContext *tclhCtxP) {
    if (tclhCtxP == NULL) {
        return Tclh_LibInit(ip, NULL);
    }
    return TCL_OK; /* Must have been already initialized */
}

Tcl_Obj *
Tclh_NsQualifyNameObj(Tcl_Interp *ip, Tcl_Obj *nameObj, const char *defaultNsP)
{
    const char *nameP;
    Tcl_Namespace *nsP;
    Tcl_Obj *fqnObj;
    Tcl_Size nameLen;

    nameP = Tcl_GetStringFromObj(nameObj, &nameLen);
    if (Tclh_NsIsFQN(nameP))
        return nameObj;

    if (defaultNsP == NULL) {
        nsP = Tcl_GetCurrentNamespace(ip);
        TCLH_ASSERT(nsP);
        defaultNsP = nsP->fullName;
    }

    fqnObj = Tcl_NewStringObj(defaultNsP, -1);
    /* Append '::' only if not global namespace else we'll get :::: */
    if (!Tclh_NsIsGlobalNs(defaultNsP))
        Tcl_AppendToObj(fqnObj, "::", 2);
    Tcl_AppendToObj(fqnObj, nameP, nameLen);
    return fqnObj;
}

const char *
Tclh_NsQualifyName(Tcl_Interp *ip, const char *nameP, Tcl_Size nameLen, Tcl_DString *dsP, const char *defaultNsP)
{
    Tcl_Namespace *nsP;

    Tcl_DStringInit(dsP); /* Init BEFORE return below since caller will Reset it */

    if (Tclh_NsIsFQN(nameP)) {
        if (nameLen < 0)
            return nameP;
        /* Name is not null terminated. We need to make it so. */
        return Tcl_DStringAppend(dsP, nameP, nameLen);
    }

    /* Qualify with current namespace */
    if (defaultNsP == NULL) {
        nsP = Tcl_GetCurrentNamespace(ip);
        TCLH_ASSERT(nsP);
        defaultNsP = nsP->fullName;
    }

    Tcl_DStringAppend(dsP, defaultNsP, -1);
    /* Append '::' only if not global namespace else we'll get :::: */
    if (!Tclh_NsIsGlobalNs(defaultNsP))
        Tcl_DStringAppend(dsP, "::", 2);
    Tcl_DStringAppend(dsP, nameP, nameLen);
    return Tcl_DStringValue(dsP);
}



/* TBD - test with sandbox */
int
Tclh_NsTailPos(const char *nameP)
{
    const char *tailP;

    /* Go to end. TBD - +strlen() intrinsic faster? */
    for (tailP = nameP;  *tailP != '\0';  tailP++) {
        /* empty body */
    }

    while (tailP > (nameP+1)) {
        if (tailP[-1] == ':' && tailP[-2] == ':')
            return (int) (tailP - nameP);
        --tailP;
    }
    /* Could not find a :: */
    return 0;
}