/*
 * Implements a MIB tree as a singly linked list, stored in lexicographic order.
 */

#ifndef _MIBLIST_H
#define _MIBLIST_H

#include "list.h"
#include "mib.h"

#ifdef __cplusplus
extern "C" {
#endif 

LIST *miblistnew(int size);
void miblistclear(LIST *l);
void miblistfree(LIST *l);

/* *data is a user-supplied space to hold the data of the MIB node. size
   refers to the length of this supplied space; and may be set to 0 for
   interger/gauge/counter/timertick types as it will default to 4. */
MIB *miblistadd(LIST *l, char *oidstr, unsigned char dataType, char access,
	void *data, int size);

MIB *miblistput(LIST *l, MIB *mib);
Boolean miblistdel(LIST *l);

MIB *miblistset(LIST *l, OID *oid, void *u, int size);
MIB *miblistgooid(LIST *l, OID *oid);
MIB *miblistgetthis(LIST *l);
MIB *miblistgetnext(LIST *l);
MIB *miblistgetprev(LIST *l);
MIB *miblistgohead(LIST *l);
MIB *miblistgotail(LIST *l);
MIB *miblistgonext(LIST *l);

#ifdef __cplusplus
}
#endif

#endif
