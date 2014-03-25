/**
 * @file 
 * @brief 
 *
 * @author  Anton Kozlov 
 * @date    23.03.2014
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "lnHist.h"

int lnHistInit(struct lnHistory *lnHist) {
    lnHist->max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
    lnHist->len = 0;
    lnHist->history = NULL;

    return 0;
}

/* Using a circular buffer is smarter, but a bit more complex to handle. */
int lnHistAddTail(struct lnHistory *lnHist, const char *line) {
    char *linecopy;

    if (lnHist->max_len == 0) return 0;
    if (lnHist->history == NULL) {
        lnHist->history = malloc(sizeof(char*)*lnHist->max_len);
        if (lnHist->history == NULL) return 0;
        memset(lnHist->history,0,(sizeof(char*)*lnHist->max_len));
    }
    linecopy = strdup(line);
    if (!linecopy) return 0;
    if (lnHist->len == lnHist->max_len) {
        free(lnHist->history[0]);
        memmove(lnHist->history,lnHist->history+1,sizeof(char*)*(lnHist->max_len-1));
        lnHist->len--;
    }
    lnHist->history[lnHist->len] = linecopy;
    lnHist->len++;
    return 1;
}

static int lnHistCheckRange(struct lnHistory *lnHist, size_t ind) {
    return ind < 0 || ind >= lnHist->len ?  -ERANGE : 0;
}

int lnHistGet(struct lnHistory *lnHist, size_t ind, char *buf, size_t len) {
    int ret;

    if ((ret = lnHistCheckRange(lnHist, ind))) {
	return ret;
    }

    strncpy(buf, lnHist->history[ind], len);
    ret = strlen(lnHist->history[ind]);
    return ret < len ? ret : len;
}

int lnHistSetMaxLen(struct lnHistory *lnHist, size_t len) {
    char **new;

    if (len < 1) return 0;
    if (lnHist->history) {
        int tocopy = lnHist->len;

        new = malloc(sizeof(char*)*len);
        if (new == NULL) return 0;

        /* If we can't copy everything, free the elements we'll not use. */
        if (len < tocopy) {
            int j;

            for (j = 0; j < tocopy-len; j++) free(lnHist->history[j]);
            tocopy = len;
        }
        memset(new,0,sizeof(char*)*len);
        memcpy(new,lnHist->history+(lnHist->len-tocopy), sizeof(char*)*tocopy);
        free(lnHist->history);
        lnHist->history = new;
    }
    lnHist->max_len = len;
    if (lnHist->len > lnHist->max_len)
        lnHist->len = lnHist->max_len;
    return 1;
}

/* Free the history, but does not reset it. Only used when we have to
 * exit() to avoid memory leaks are reported by valgrind & co. */
void lnHistDeinit(struct lnHistory *lnHist) {
    if (lnHist->history) {
        int j;

        for (j = 0; j < lnHist->len; j++)
            free(lnHist->history[j]);
        free(lnHist->history);
    }
}

