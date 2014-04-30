/**
 * @file 
 * @brief
 *
 * @author  Anton Kozlov 
 * @date    25.03.2014
 */

#ifndef LN_COMPL_H_
#define LN_COMPL_H_

struct lnComplVariants {
  size_t len;
  char **cvec;
};

typedef void (*lnComplCallback_t)(struct lnComplVariants *lnComplVars, const char *input);

extern void lnComplInit(struct lnComplVariants *lnComplVars);
extern void lnComplAdd(struct lnComplVariants *lnComplVars, const char *str);
extern void lnComplFree(struct lnComplVariants *lnComplVars);
extern const char *lnComplGet(struct lnComplVariants *lnComplVars, int n);

#endif /* LN_COMPL_H_ */

