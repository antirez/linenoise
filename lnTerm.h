/**
 * @file 
 * @brief 
 *
 * @author  Anton Kozlov 
 * @date    22.03.2014
 */

#include <sys/types.h>

// TODO hide from here
#include <termios.h>
struct lnTerminal {
	int fd;
	struct termios orig_termios;
	char rawmode;
};

extern int lnTermGetColumns(struct lnTerminal *lnTerm);
extern int lnTermSavePrepare(struct lnTerminal *lnTerm);
extern void lnTermResotre(struct lnTerminal *lnTerm);
extern int lnTermWrite(struct lnTerminal *lnTerm, const char *buf, size_t len);
extern int lnTermRead(struct lnTerminal *lnTerm, char *buf, size_t len);
void lnTermBeep(struct lnTerminal *lnTerm);

extern void lnTermClearScreen(struct lnTerminal *lnTerm);
extern void lnTermClearLineRight(struct lnTerminal *lnTerm);
extern void lnTermCursorSet(struct lnTerminal *lnTerm, unsigned int pos);
extern void lnTermCursorLineAdd(struct lnTerminal *lnTerm, int l);

