/**
 * @file 
 * @brief 
 *
 * @author  Anton Kozlov 
 * @date    23.03.2014
 */

#ifndef LINENOISE_HIST_H_
#define LINENOISE_HIST_H_

#include <sys/types.h>

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100
struct lnHistory { 
    int history_max_len;// = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
    int history_len; //= 0;
    char **history; // = NULL;

    int history_index;  /* The history index we are currently editing. */
};

extern int lnHistInit(struct lnHistory *lnHist);
extern int lnHistAdd(struct lnHistory *lnHist, const char *line);
extern int lnHistSeek(struct lnHistory *lnHist, int d);
extern int lnHistGet(struct lnHistory *lnHist, char *buf, size_t len);

#endif /* LINENOISE_HIST_H_ */

