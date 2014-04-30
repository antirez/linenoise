/* linenoise.h -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * See linenoise.c for more information.
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
