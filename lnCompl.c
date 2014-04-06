/**
 * @file 
 * @brief 
 *
 * @author  Anton Kozlov 
 * @date    06.04.2014
 */

#include <stdlib.h>
#include <string.h>

#include "lnCompl.h"

void lnComplInit(struct lnComplVariants *lnComplVars) {
	lnComplVars->len = 0;
	lnComplVars->cvec = NULL;
}

/* This function is used by the callback function registered by the user
 * in order to add completion options given the input string when the
 * user typed <tab>. See the example.c source code for a very easy to
 * understand example. */
void lnComplAdd(struct lnComplVariants *lnComplVars, const char *str) {
    size_t len = strlen(str);
    char *copy = malloc(len+1);
    memcpy(copy,str,len+1);
    lnComplVars->cvec = realloc(lnComplVars->cvec,sizeof(char*)*(lnComplVars->len+1));
    lnComplVars->cvec[lnComplVars->len++] = copy;
}

void lnComplFree(struct lnComplVariants *lnComplVars) {
    size_t i;
    for (i = 0; i < lnComplVars->len; i++)
        free(lnComplVars->cvec[i]);
    if (lnComplVars->cvec != NULL)
        free(lnComplVars->cvec);
}

const char *lnComplGet(struct lnComplVariants *lnComplVars, int n) {
	if (n < 0 || n >= lnComplVars->len)
		return NULL;

	return lnComplVars->cvec[n];
}
