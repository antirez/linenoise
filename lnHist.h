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

#define LN_HIST_SEEK_SET 0
#define LN_HIST_SEEK_CUR 1

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100

struct lnHistory { 
    int max_len;
    char **history;
    int len;
};

extern int lnHistInit(struct lnHistory *lnHist);
extern int lnHistAddTail(struct lnHistory *lnHist, const char *line);
extern int lnHistGet(struct lnHistory *lnHist, size_t ind, char *buf, size_t len);
extern int lnHistSetMaxLen(struct lnHistory *lnHist, size_t len);
extern void lnHistDeinit(struct lnHistory *lnHist);

#endif /* LINENOISE_HIST_H_ */

