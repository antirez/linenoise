/**
 * @file 
 * @brief 
 *
 * @author  Anton Kozlov 
 * @date    22.03.2014
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdarg.h>

#include "lnTerm.h"

/* Raw mode: 1960 magic shit. */
int lnTermSavePrepare(struct lnTerminal *lnTerm) {
    struct termios raw;

	memset(lnTerm, 0, sizeof(*lnTerm));
	lnTerm->fd = STDIN_FILENO;

    if (tcgetattr(lnTerm->fd, &lnTerm->orig_termios) == -1) goto fatal;

    raw = lnTerm->orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(lnTerm->fd, TCSAFLUSH, &raw) < 0) goto fatal;
    lnTerm->rawmode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

void lnTermResotre(struct lnTerminal *lnTerm) {
    /* Don't even check the return value as it's too late. */
    if (lnTerm->rawmode && tcsetattr(lnTerm->fd, TCSAFLUSH, &lnTerm->orig_termios) != -1)
        lnTerm->rawmode = 0;
}

int lnTermWrite(struct lnTerminal *lnTerm, const char *buf, size_t len) {
	return write(lnTerm->fd, buf, len);
}

int lnTermRead(struct lnTerminal *lnTerm, char *buf, size_t len) {
	return read(lnTerm->fd, buf, len);
}

static int lnTermVtSeq(struct lnTerminal *lnTerm, const char *fmt, ...) {
	char seq[64];
	int len;
	va_list va;

	strcpy(seq, "\x1b[");

	va_start(va, fmt);
	len = 2 + vsnprintf(seq + 2, sizeof(seq) - 2, fmt, va);
	va_end(va);

	lnTermWrite(lnTerm, seq, len);
	return len;
}

void lnTermClearScreen(struct lnTerminal *lnTerm) {
	lnTermVtSeq(lnTerm, "H");
	lnTermVtSeq(lnTerm, "2J");
}

void lnTermClearLineRight(struct lnTerminal *lnTerm) {
	lnTermVtSeq(lnTerm, "0K");
}

void lnTermCursorSet(struct lnTerminal *lnTerm, unsigned int pos) {
	lnTermVtSeq(lnTerm, "0G");
	if (pos) 
		lnTermVtSeq(lnTerm, "%dC", pos);
}

void lnTermCursorLineAdd(struct lnTerminal *lnTerm, int l) {
	if (l >= 0) 
		lnTermVtSeq(lnTerm, "%dB", l);
	else 
		lnTermVtSeq(lnTerm, "%dA", -l);
}

int lnTermGetColumns(struct lnTerminal *lnTerm) {
    struct winsize ws;

    if (ioctl(lnTerm->fd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) return 80;
    return ws.ws_col;
}

void lnTermBeep(struct lnTerminal *lnTerm) {
    fprintf(stderr, "\x7");
    fflush(stderr);
}
