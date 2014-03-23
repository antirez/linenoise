/**
 * @file 
 * @brief 
 *
 * @author  Anton Kozlov 
 * @date    23.03.2014
 */

#include <string.h>
#include <stdlib.h>

#include "lnHist.h"

int lnHistInit(struct lnHistory *lnHist) {
    lnHist->history_max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
    lnHist->history_len = 0;
    lnHist->history = NULL;
    lnHist->history_index = 0;

    return 0;
}

/* Using a circular buffer is smarter, but a bit more complex to handle. */
int lnHistAdd(struct lnHistory *lnHist, const char *line) {
    char *linecopy;

    if (lnHist->history_max_len == 0) return 0;
    if (lnHist->history == NULL) {
        lnHist->history = malloc(sizeof(char*)*lnHist->history_max_len);
        if (lnHist->history == NULL) return 0;
        memset(lnHist->history,0,(sizeof(char*)*lnHist->history_max_len));
    }
    linecopy = strdup(line);
    if (!linecopy) return 0;
    if (lnHist->history_len == lnHist->history_max_len) {
        free(lnHist->history[0]);
        memmove(lnHist->history,lnHist->history+1,sizeof(char*)*(lnHist->history_max_len-1));
        lnHist->history_len--;
    }
    lnHist->history[lnHist->history_len] = linecopy;
    lnHist->history_len++;
    return 1;
}

