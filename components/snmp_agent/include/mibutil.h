/*
 * Utility functions to file and display MIB data, mainly for Windows
 * and *nix.
 */

#ifndef _MIBUTIL_H
#define _MIBUTIL_H

#include <stdio.h>
#include "octet.h"
#include "miblist.h"
#include "varbind.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Prints the MIB data as a keylist string in s. */
void mibprint(MIB *thismib, char *s);

/* Scans a keylist string of MIB data into the MIB structure. */
int mibscan(MIB *thismib, char *s);

/* Reads from a file and populates a MIB list. */
int miblistread(LIST *l, char *fn);

void miblistprint(LIST *l, FILE *f);
int miblistwrite(LIST *l, char *fn);
void vblistPrint(struct messageStruct *vblist, FILE *f);

/* Display packet, mainly used for debugging. */
void showMessage(struct messageStruct *pkt);

#ifdef __cplusplus
}
#endif

#endif
