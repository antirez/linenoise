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

extern int lnTermSavePrepare(struct lnTerminal *lnTerm);

void lnTermResotre(struct lnTerminal *lnTerm);

int lnTermWrite(struct lnTerminal *lnTerm, const char *buf, size_t len);

int lnTermRead(struct lnTerminal *lnTerm, char *buf, size_t len);
