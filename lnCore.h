
#ifndef LN_CORE_H_
#define LN_CORE_H_

#define LN_MULTILINE 0x01

extern int linenoiseEdit(struct lnTerminal *lnTerm, struct lnHistory *lnHist, 
        lnComplCallback_t lnComplCb,
        const char *prompt, int opt, char *buf, size_t buflen);

#endif /* LN_CORE_H_ */

