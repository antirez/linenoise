/**
 * @file 
 * @brief 
 *
 * @author  Anton Kozlov 
 * @date    25.03.2014
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "lnTerm.h"
#include "lnHist.h"
#include "lnCompl.h"

#include "linenoise.h"

extern int mlmode;
extern linenoiseCompletionCallback *completionCallback;

/* Single line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal. */
static void refreshSingleLine(struct linenoiseState *l) {
    char *buf = l->buf;
    size_t len = l->len;
    size_t pos = l->pos;

    while (l->plen + pos >= l->cols) {
        buf++;
        len--;
        pos--;
    }

    while (l->plen + len > l->cols) {
        len--;
    }

    /* Cursor to left edge */
    lnTermCursorSet(l->lnTerm, 0);
    /* Write the prompt and the current buffer content */
    lnTermWrite(l->lnTerm, l->prompt, l->plen);
    lnTermWrite(l->lnTerm, buf, len);
    /* Erase to right */
    lnTermClearLineRight(l->lnTerm);
    /* Move cursor to original position. */
    lnTermCursorSet(l->lnTerm, pos + l->plen);
}

/* Multi line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal. */
static void refreshMultiLine(struct linenoiseState *l) {
    int plen = strlen(l->prompt);
    int rows = (plen+l->len+l->cols-1)/l->cols; /* rows used by current buf. */
    int rpos = (plen+l->oldpos+l->cols)/l->cols; /* cursor relative row. */
    int rpos2; /* rpos after refresh. */
    int old_rows = l->maxrows;
    int j;

    /* Update maxrows if needed. */
    if (rows > (int)l->maxrows) l->maxrows = rows;

#ifdef LN_DEBUG
    FILE *fp = fopen("/tmp/debug.txt","a");
    fprintf(fp,"[%d %d %d] p: %d, rows: %d, rpos: %d, max: %d, oldmax: %d",
        (int)l->len,(int)l->pos,(int)l->oldpos,plen,rows,rpos,(int)l->maxrows,old_rows);
#endif

    /* First step: clear all the lines used before. To do so start by
     * going to the last row. */
    if (old_rows-rpos > 0) {
#ifdef LN_DEBUG
        fprintf(fp,", go down %d", old_rows-rpos);
#endif
        lnTermCursorLineAdd(l->lnTerm, old_rows - rpos);
    }

    /* Now for every row clear it, go up. */
    for (j = 0; j < old_rows-1; j++) {
#ifdef LN_DEBUG
        fprintf(fp,", clear+up");
#endif
        lnTermCursorSet(l->lnTerm, 0);
        lnTermClearLineRight(l->lnTerm);
        lnTermCursorLineAdd(l->lnTerm, -1);
    }

    /* Clean the top line. */
#ifdef LN_DEBUG
    fprintf(fp,", clear");
#endif
    lnTermCursorSet(l->lnTerm, 0);
    lnTermClearLineRight(l->lnTerm);

    /* Write the prompt and the current buffer content */
    if (lnTermWrite(l->lnTerm,l->prompt,strlen(l->prompt)) == -1) return;
    if (lnTermWrite(l->lnTerm,l->buf,l->len) == -1) return;

    /* If we are at the very end of the screen with our prompt, we need to
     * emit a newline and move the prompt to the first column. */
    if (l->pos &&
        l->pos == l->len &&
        (l->pos+plen) % l->cols == 0)
    {
#ifdef LN_DEBUG
        fprintf(fp,", <newline>");
#endif
        if (lnTermWrite(l->lnTerm,"\n",1) == -1) return;
        lnTermCursorSet(l->lnTerm, 0);
        rows++;
        if (rows > (int)l->maxrows) l->maxrows = rows;
    }

    /* Move cursor to right position. */
    rpos2 = (plen+l->pos+l->cols)/l->cols; /* current cursor relative row. */
#ifdef LN_DEBUG
    fprintf(fp,", rpos2 %d", rpos2);
#endif
    /* Go up till we reach the expected positon. */
    if (rows-rpos2 > 0) {
#ifdef LN_DEBUG
        fprintf(fp,", go-up %d", rows-rpos2);
#endif
        lnTermCursorLineAdd(l->lnTerm, - (rows - rpos2));
    }
    /* Set column. */
#ifdef LN_DEBUG
    fprintf(fp,", set col %d", ((plen+(int)l->pos) % (int)l->cols));
#endif
    lnTermCursorSet(l->lnTerm, ((plen+(int)l->pos) % (int)l->cols));

    l->oldpos = l->pos;

#ifdef LN_DEBUG
    fprintf(fp,"\n");
    fclose(fp);
#endif
}

/* Calls the two low level functions refreshSingleLine() or
 * refreshMultiLine() according to the selected mode. */
static void refreshLine(struct linenoiseState *l) {
    if (mlmode)
        refreshMultiLine(l);
    else
        refreshSingleLine(l);
}

/* Insert the character 'c' at cursor current position.
 *
 * On error writing to the terminal -1 is returned, otherwise 0. */
static int linenoiseEditInsert(struct linenoiseState *l, char c) {
    if (l->len < l->buflen) {
        if (l->len == l->pos) {
            l->buf[l->pos] = c;
            l->pos++;
            l->len++;
            l->buf[l->len] = '\0';
            if ((!mlmode && l->plen+l->len < l->cols) /* || mlmode */) {
                /* Avoid a full update of the line in the
                 * trivial case. */
                if (lnTermWrite(l->lnTerm, &c, 1) == -1) return -1;
            } else {
                refreshLine(l);
            }
        } else {
            memmove(l->buf+l->pos+1,l->buf+l->pos,l->len-l->pos);
            l->buf[l->pos] = c;
            l->len++;
            l->pos++;
            l->buf[l->len] = '\0';
            refreshLine(l);
        }
    }
    return 0;
}

/* Move cursor on the left. */
static void linenoiseEditMoveLeft(struct linenoiseState *l) {
    if (l->pos > 0) {
        l->pos--;
        refreshLine(l);
    }
}

/* Move cursor on the right. */
static void linenoiseEditMoveRight(struct linenoiseState *l) {
    if (l->pos != l->len) {
        l->pos++;
        refreshLine(l);
    }
}

/* Substitute the currently edited line with the next or previous history
 * entry as specified by 'dir'. */
#define LINENOISE_HISTORY_NEXT 0
#define LINENOISE_HISTORY_PREV 1
static void linenoiseEditHistoryNext(struct linenoiseState *l, int dir) {
    int d = dir == LINENOISE_HISTORY_PREV ? -1 : 1;
    int len;

    if (0 <= (len = lnHistGet(l->lnHist, l->history_index + d, l->buf, l->buflen))
            || (len = 0, l->history_index + d == l->lnHist->len)) {
        l->history_index += d;
        l->len = l->pos = len;
    }
    refreshLine(l);
}

/* Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key. */
static void linenoiseEditDelete(struct linenoiseState *l) {
    if (l->len > 0 && l->pos < l->len) {
        memmove(l->buf+l->pos,l->buf+l->pos+1,l->len-l->pos-1);
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

/* Backspace implementation. */
static void linenoiseEditBackspace(struct linenoiseState *l) {
    if (l->pos > 0 && l->len > 0) {
        memmove(l->buf+l->pos-1,l->buf+l->pos,l->len-l->pos);
        l->pos--;
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

/* Delete the previosu word, maintaining the cursor at the start of the
 * current word. */
static void linenoiseEditDeletePrevWord(struct linenoiseState *l) {
    size_t old_pos = l->pos;
    size_t diff;

    while (l->pos > 0 && l->buf[l->pos-1] == ' ')
        l->pos--;
    while (l->pos > 0 && l->buf[l->pos-1] != ' ')
        l->pos--;
    diff = old_pos - l->pos;
    memmove(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
    l->len -= diff;
    refreshLine(l);
}

static void linenoiseStateInit(struct linenoiseState *l, struct lnTerminal *lnTerm,
	struct lnHistory *lnHist, linenoiseCompletionCallback complCb,
        char *buf, size_t buflen, const char *prompt) {

    /* Populate the linenoise state that we pass to functions implementing
     * specific editing functionalities. */
    l->lnTerm = lnTerm;
    l->lnHist = lnHist;
    l->completionCallback = complCb;

    l->buf = buf;
    l->buflen = buflen;
    l->prompt = prompt;
    l->plen = strlen(prompt);

    l->oldpos = l->pos = 0;
    l->len = 0;
    l->cols = lnTermGetColumns(lnTerm);
    l->maxrows = 0;
    l->history_index = lnHist->len;
}

/* This is an helper function for linenoiseEdit() and is called when the
 * user types the <tab> key in order to complete the string currently in the
 * input.
 * 
 * The state of the editing is encapsulated into the pointed linenoiseState
 * structure as described in the structure definition. */
static int completeLine(struct linenoiseState *ls) {
    struct lnComplVariants lnComplVars;
    int nread, nwritten;
    int stop, i;
    char c = 0;

    lnComplInit(&lnComplVars);
    ls->completionCallback(ls->buf,&lnComplVars);

    stop = 0;
    i = 0;
    while(!stop) {
        const char *compl_line;

        if ((compl_line = lnComplGet(&lnComplVars, i))) {
            struct linenoiseState saved = *ls;

            ls->buf = (char *) compl_line;
            ls->len = ls->pos = strlen(compl_line);
            refreshLine(ls);
            ls->len = saved.len;
            ls->pos = saved.pos;
            ls->buf = saved.buf;
        } else {
            i = -1;
            refreshLine(ls);
            lnTermBeep(ls->lnTerm);
        }

        nread = lnTermRead(ls->lnTerm,&c,1);
        if (nread <= 0) {
            lnComplFree(&lnComplVars);
            return -1;
        }

        switch(c) {
            case 9: /* tab */
                i++;
                break;
            case 27: /* escape */
                /* Re-show original buffer */
                if (i >= 0) refreshLine(ls);
                stop = 1;
                break;
            default:
                /* Update buffer and return */
                if (i >= 0) {
                    nwritten = snprintf(ls->buf,ls->buflen,"%s",lnComplGet(&lnComplVars, i));
                    ls->len = ls->pos = nwritten;
                }
                stop = 1;
                break;
        }
    }

    lnComplFree(&lnComplVars);
    return c; /* Return last read character */
}


/* This function is the core of the line editing capability of linenoise.
 *
 * The resulting string is put into 'buf' when the user type enter, or
 * when ctrl+d is typed.
 *
 * The function returns the length of the current buffer. */
int linenoiseEdit(struct lnTerminal *lnTerm, struct lnHistory *lnHist, 
        linenoiseCompletionCallback *complCb,
	char *buf, size_t buflen, const char *prompt) {
    struct linenoiseState l;

    linenoiseStateInit(&l, lnTerm, lnHist, complCb, buf, buflen, prompt);

    /* Buffer starts empty. */
    buf[0] = '\0';
    buflen--; /* Make sure there is always space for the nulterm */

    refreshLine(&l);

    while(1) {
        char c;
        int nread;
        char seq[2], seq2[2];

        nread = lnTermRead(lnTerm, &c, 1);
        if (nread <= 0) return l.len;

        /* Only autocomplete when the callback is set. It returns < 0 when
         * there was an error reading from fd. Otherwise it will return the
         * character that should be handled next. */
        if (c == 9 && complCb != NULL) {
            c = completeLine(&l);
            /* Return on errors */
            if (c < 0) return l.len;
            /* Read next character when 0 */
            if (c == 0) continue;
        }

        switch(c) {
        case 13:    /* enter */
            return (int)l.len;
        case 3:     /* ctrl-c */
            errno = EAGAIN;
            return -1;
        case 127:   /* backspace */
        case 8:     /* ctrl-h */
            linenoiseEditBackspace(&l);
            break;
        case 4:     /* ctrl-d, remove char at right of cursor, or of the
                       line is empty, act as end-of-file. */
            if (l.len > 0) {
                linenoiseEditDelete(&l);
            } else {
                return -1;
            }
            break;
        case 20:    /* ctrl-t, swaps current character with previous. */
            if (l.pos > 0 && l.pos < l.len) {
                int aux = buf[l.pos-1];
                buf[l.pos-1] = buf[l.pos];
                buf[l.pos] = aux;
                if (l.pos != l.len-1) l.pos++;
                refreshLine(&l);
            }
            break;
        case 2:     /* ctrl-b */
            linenoiseEditMoveLeft(&l);
            break;
        case 6:     /* ctrl-f */
            linenoiseEditMoveRight(&l);
            break;
        case 16:    /* ctrl-p */
            linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_PREV);
            break;
        case 14:    /* ctrl-n */
            linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_NEXT);
            break;
        case 27:    /* escape sequence */
            /* Read the next two bytes representing the escape sequence. */
            if (lnTermRead(lnTerm, seq, 2) == -1) break;

            if (seq[0] == 91 && seq[1] == 68) {
                /* Left arrow */
                linenoiseEditMoveLeft(&l);
            } else if (seq[0] == 91 && seq[1] == 67) {
                /* Right arrow */
                linenoiseEditMoveRight(&l);
            } else if (seq[0] == 91 && (seq[1] == 65 || seq[1] == 66)) {
                /* Up and Down arrows */
                linenoiseEditHistoryNext(&l,
                    (seq[1] == 65) ? LINENOISE_HISTORY_PREV :
                                     LINENOISE_HISTORY_NEXT);
            } else if (seq[0] == 91 && seq[1] > 48 && seq[1] < 55) {
                /* extended escape, read additional two bytes. */
                if (lnTermRead(lnTerm, seq2, 2) == -1) break;
                if (seq[1] == 51 && seq2[0] == 126) {
                    /* Delete key. */
                    linenoiseEditDelete(&l);
                }
            }
            break;
        default:
            if (linenoiseEditInsert(&l,c)) return -1;
            break;
        case 21: /* Ctrl+u, delete the whole line. */
            buf[0] = '\0';
            l.pos = l.len = 0;
            refreshLine(&l);
            break;
        case 11: /* Ctrl+k, delete from current to end of line. */
            buf[l.pos] = '\0';
            l.len = l.pos;
            refreshLine(&l);
            break;
        case 1: /* Ctrl+a, go to the start of the line */
            l.pos = 0;
            refreshLine(&l);
            break;
        case 5: /* ctrl+e, go to the end of the line */
            l.pos = l.len;
            refreshLine(&l);
            break;
        case 12: /* ctrl+l, clear screen */
            lnTermClearScreen(lnTerm);
            refreshLine(&l);
            break;
        case 23: /* ctrl+w, delete previous word */
            linenoiseEditDeletePrevWord(&l);
            break;
        }
    }
    return l.len;
}
