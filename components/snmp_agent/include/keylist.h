/*
 * Implements a key-value-pair list.
 */

#ifndef _KEYLIST_H
#define _KEYLIST_H

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define KEY_SIZE 32
#define VAL_SIZE 256

typedef struct {
	char key[KEY_SIZE];
	char val[VAL_SIZE];
} KEY;

char *strtrim(char *s);

LIST *keylistnew(void);
void keylistclear(LIST *l);
void keylistfree(LIST *l);
void keylistprint(LIST *l, FILE *f);
int keylistwrite(LIST *l, char *fn);
int keylistread(LIST *l, char *fn);

int keylistgetval(char *fn, char *key, char *val);
int keylistsetval(char *fn, char *key, char *val);
KEY *keylistadd(LIST *l, char *key, char *val);
KEY *keylistset(LIST *l, char *key, char *val);
Boolean keylistdel(LIST *l);

KEY *keylistgokey(LIST *l, char *key);
KEY *keylistgovalue(LIST *l, char *val);

int keylistsize(LIST *l);
KEY *keylistgetthis(LIST *l);
KEY *keylistgetnext(LIST *l);
KEY *keylistgetprev(LIST *l);
KEY *keylistgohead(LIST *l);
KEY *keylistgotail(LIST *l);
KEY *keylistgonext(LIST *l);

#ifdef __cplusplus
}
#endif

#endif
