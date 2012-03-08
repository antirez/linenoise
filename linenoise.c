/* linenoise.c -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * You can find the latest source code at:
 * 
 *   http://github.com/antirez/linenoise
 *
 * Does a number of crazy assumptions that happen to be true in 99.9999% of
 * the 2010 UNIX computers around.
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
 * 
 * ------------------------------------------------------------------------
 *
 * References:
 * - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - http://www.3waylabs.com/nw/WWW/products/wizcon/vt220.html
 *
 * Todo list:
 * - Switch to gets() if $TERM is something we can't support.
 * - Filter bogus Ctrl+<char> combinations.
 * - Win32 support
 *
 * Bloat:
 * - Completion?
 * - History search like Ctrl+r in readline?
 *
 * List of escape sequences used by this program, we do everything just
 * with three sequences. In order to be so cheap we may have some
 * flickering effect with some slow terminal, but the lesser sequences
 * the more compatible.
 *
 * CHA (Cursor Horizontal Absolute)
 *    Sequence: ESC [ n G
 *    Effect: moves cursor to column n
 *
 * EL (Erase Line)
 *    Sequence: ESC [ n K
 *    Effect: if n is 0 or missing, clear from cursor to end of line
 *    Effect: if n is 1, clear from beginning of line to cursor
 *    Effect: if n is 2, clear entire line
 *
 * CUF (CUrsor Forward)
 *    Sequence: ESC [ n C
 *    Effect: moves cursor forward of n chars
 *
 * The following are used to clear the screen: ESC [ H ESC [ 2 J
 * This is actually composed of two sequences:
 *
 * cursorhome
 *    Sequence: ESC [ H
 *    Effect: moves the cursor to upper left corner
 *
 * ED2 (Clear entire screen)
 *    Sequence: ESC [ 2 J
 *    Effect: clear the whole screen
 * 
 */

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termkey.h>
#include "linenoise.h"

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100
#define LINENOISE_MAX_LINE 4096
static char *unsupported_term[] = {"dumb","cons25",NULL};
static linenoiseCompletionCallback *completionCallback = NULL;

static int atexit_registered = 0; /* register atexit just 1 time */
static int history_max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
static int history_len = 0;
char **history = NULL;
static TermKey *tk = NULL;

/* Some useful termkey utilities */
static int termkey_key_is_keysym(TermKeyKey *key, TermKeySym sym, int mod)
{
  return key->type      == TERMKEY_TYPE_KEYSYM &&
         key->code.sym  == sym &&
         key->modifiers == mod;
}

static void linenoiseAtExit(void);
int linenoiseHistoryAdd(const char *line);

static int isUnsupportedTerm(void) {
    char *term = getenv("TERM");
    int j;

    if (term == NULL) return 0;
    for (j = 0; unsupported_term[j]; j++)
        if (!strcasecmp(term,unsupported_term[j])) return 1;
    return 0;
}

static void freeHistory(void) {
    if (history) {
        int j;

        for (j = 0; j < history_len; j++)
            free(history[j]);
        free(history);
    }
}

/* At exit we'll try to fix the terminal to the initial conditions. */
static void linenoiseAtExit(void) {
    termkey_stop(tk);
    freeHistory();
}

static int getColumns(void) {
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1) return 80;
    return ws.ws_col;
}

static void refreshLine(int fd, const char *prompt, char *buf, size_t len, size_t pos, size_t cols) {
    char seq[64];
    size_t plen = strlen(prompt);
    
    while((plen+pos) >= cols) {
        buf++;
        len--;
        pos--;
    }
    while (plen+len > cols) {
        len--;
    }

    /* Cursor to left edge */
    snprintf(seq,64,"\x1b[0G");
    if (write(fd,seq,strlen(seq)) == -1) return;
    /* Write the prompt and the current buffer content */
    if (write(fd,prompt,strlen(prompt)) == -1) return;
    if (write(fd,buf,len) == -1) return;
    /* Erase to right */
    snprintf(seq,64,"\x1b[0K");
    if (write(fd,seq,strlen(seq)) == -1) return;
    /* Move cursor to original position. */
    snprintf(seq,64,"\x1b[0G\x1b[%dC", (int)(pos+plen));
    if (write(fd,seq,strlen(seq)) == -1) return;
}

static void beep() {
    fprintf(stderr, "\x7");
    fflush(stderr);
}

static void freeCompletions(linenoiseCompletions *lc) {
    size_t i;
    for (i = 0; i < lc->len; i++)
        free(lc->cvec[i]);
    if (lc->cvec != NULL)
        free(lc->cvec);
}

static int completeLine(TermKey *tk, TermKeyKey *keyp, const char *prompt, char *buf, size_t buflen, size_t *len, size_t *pos, size_t cols) {
    linenoiseCompletions lc = { 0, NULL };
    int nwritten;

    int fd = termkey_get_fd(tk);

    completionCallback(buf,&lc);
    if (lc.len == 0) {
        beep();
    } else {
        size_t stop = 0, i = 0;
        size_t clen;

        while(!stop) {
            /* Show completion or original buffer */
            if (i < lc.len) {
                clen = strlen(lc.cvec[i]);
                refreshLine(fd,prompt,lc.cvec[i],clen,clen,cols);
            } else {
                refreshLine(fd,prompt,buf,*len,*pos,cols);
            }

            if(termkey_waitkey(tk, keyp) != TERMKEY_RES_KEY) {
                freeCompletions(&lc);
                return -1;
            }

            if(termkey_key_is_keysym(keyp, TERMKEY_SYM_TAB, 0)) {
                i = (i+1) % (lc.len+1);
                if (i == lc.len) beep();
            }
            else if(termkey_key_is_keysym(keyp, TERMKEY_SYM_ESCAPE, 0)) {
                /* Re-show original buffer */
                if (i < lc.len) {
                    refreshLine(fd,prompt,buf,*len,*pos,cols);
                }
                stop = 1;
            }
            else {
                /* Update buffer and return */
                if (i < lc.len) {
                    nwritten = snprintf(buf,buflen,"%s",lc.cvec[i]);
                    *len = *pos = nwritten;
                }
                stop = 1;
            }
        }
    }

    freeCompletions(&lc);
    return 1;
}

void linenoiseClearScreen(void) {
    if (write(STDIN_FILENO,"\x1b[H\x1b[2J",7) <= 0) {
        /* nothing to do, just to avoid warning. */
    }
}

static int linenoisePrompt(TermKey *tk, char *buf, size_t buflen, const char *prompt) {
    size_t plen = strlen(prompt);
    size_t pos = 0;
    size_t len = 0;
    size_t cols = getColumns();
    int history_index = 0;

    int fd = termkey_get_fd(tk);

    buf[0] = '\0';
    buflen--; /* Make sure there is always space for the nulterm */

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    linenoiseHistoryAdd("");
    
    if (write(termkey_get_fd(tk),prompt,plen) == -1) return -1;
    while(1) {
        TermKeyKey key;
        if(termkey_waitkey(tk, &key) != TERMKEY_RES_KEY)
          return len;

        /* Only autocomplete when the callback is set. It returns < 0 when
         * there was an error reading from fd. Otherwise it will return the
         * character that should be handled next. */
        if (termkey_key_is_keysym(&key, TERMKEY_SYM_TAB, 0) && completionCallback != NULL) {
            int c = completeLine(tk,&key,prompt,buf,buflen,&len,&pos,cols);
            /* TODO */
            /* Return on errors */
            if (c < 0) return len;
            /* Read next character when 0 */
            if (c == 0) continue;
        }

        if(key.type == TERMKEY_TYPE_UNICODE && key.modifiers == TERMKEY_KEYMOD_CTRL) {
            switch(key.code.codepoint) {
            case 'c':
                errno = EAGAIN;
                return -1;
            case 'h':
                goto backspace;
            case 'd':
                goto delete_key;
            case 't':
                if (pos > 0 && pos < len) {
                    int aux = buf[pos-1];
                    buf[pos-1] = buf[pos];
                    buf[pos] = aux;
                    if (pos != len-1) pos++;
                    refreshLine(fd,prompt,buf,len,pos,cols);
                }
                break;
            case 'b':
                goto left_arrow;
            case 'f':
                goto right_arrow;
            case 'p':
                key.code.sym = TERMKEY_SYM_UP;
                goto up_down_arrow;
            case 'n':
                key.code.sym = TERMKEY_SYM_DOWN;
                goto up_down_arrow;
            case 'u': /* Ctrl+u, delete the whole line. */
                buf[0] = '\0';
                pos = len = 0;
                refreshLine(fd,prompt,buf,len,pos,cols);
                break;
            case 'k': /* Ctrl+k, delete from current to end of line. */
                buf[pos] = '\0';
                len = pos;
                refreshLine(fd,prompt,buf,len,pos,cols);
                break;
            case 'a':
                goto home_key;
            case 'e':
                goto end_key;
            case 'l': /* ctrl+l, clear screen */
                linenoiseClearScreen();
                refreshLine(fd,prompt,buf,len,pos,cols);
            }
        }
        else if (key.type == TERMKEY_TYPE_KEYSYM && key.modifiers == 0) {
            switch(key.code.sym) {
            case TERMKEY_SYM_ENTER:
                history_len--;
                free(history[history_len]);
                return (int)len;
            case TERMKEY_SYM_BACKSPACE:
backspace:
                if (pos > 0 && len > 0) {
                    memmove(buf+pos-1,buf+pos,len-pos);
                    pos--;
                    len--;
                    buf[len] = '\0';
                    refreshLine(fd,prompt,buf,len,pos,cols);
                }
                break;
            case TERMKEY_SYM_LEFT:
left_arrow:
                if (pos > 0) {
                    pos--;
                    refreshLine(fd,prompt,buf,len,pos,cols);
                }
                break;
            case TERMKEY_SYM_RIGHT:
right_arrow:
                if (pos != len) {
                    pos++;
                    refreshLine(fd,prompt,buf,len,pos,cols);
                }
                break;
            case TERMKEY_SYM_UP:
            case TERMKEY_SYM_DOWN:
up_down_arrow:
                /* up and down arrow: history */
                if (history_len > 1) {
                    /* Update the current history entry before to
                     * overwrite it with tne next one. */
                    free(history[history_len-1-history_index]);
                    history[history_len-1-history_index] = strdup(buf);
                    /* Show the new entry */
                    history_index += (key.code.sym == TERMKEY_SYM_UP) ? 1 : -1;
                    if (history_index < 0) {
                        history_index = 0;
                        break;
                    } else if (history_index >= history_len) {
                        history_index = history_len-1;
                        break;
                    }
                    strncpy(buf,history[history_len-1-history_index],buflen);
                    buf[buflen] = '\0';
                    len = pos = strlen(buf);
                    refreshLine(fd,prompt,buf,len,pos,cols);
                }
                break;
            case TERMKEY_SYM_DEL:
                if (len > 0 && pos < len) {
                    memmove(buf+pos,buf+pos+1,len-pos-1);
                    len--;
                    buf[len] = '\0';
                    refreshLine(fd,prompt,buf,len,pos,cols);
                }
                break;
            case TERMKEY_SYM_DELETE:
delete_key:
                if (len > 1 && pos < (len-1)) {
                    memmove(buf+pos,buf+pos+1,len-pos);
                    len--;
                    buf[len] = '\0';
                    refreshLine(fd,prompt,buf,len,pos,cols);
                } else if (len == 0) {
                    history_len--;
                    free(history[history_len]);
                    return -1;
                }
                break;
            case TERMKEY_SYM_HOME:
home_key:
                pos = 0;
                refreshLine(fd,prompt,buf,len,pos,cols);
                break;
            case TERMKEY_SYM_END:
end_key:
                pos = len;
                refreshLine(fd,prompt,buf,len,pos,cols);
                break;
            default: break;
            }
        }
        else if (key.type == TERMKEY_TYPE_UNICODE && key.modifiers == 0) {
            if (len < buflen) {
                if (len == pos) {
                    /* TODO: UTF-8 handling */
                    buf[pos] = key.utf8[0];
                    pos++;
                    len++;
                    buf[len] = '\0';
                    if (plen+len < cols) {
                        /* Avoid a full update of the line in the
                         * trivial case. */
                        if (write(fd,&key.utf8[0],1) == -1) return -1;
                    } else {
                        refreshLine(fd,prompt,buf,len,pos,cols);
                    }
                } else {
                    memmove(buf+pos+1,buf+pos,len-pos);
                    buf[pos] = key.utf8[0];
                    len++;
                    pos++;
                    buf[len] = '\0';
                    refreshLine(fd,prompt,buf,len,pos,cols);
                }
            }
        }
    }
    return len;
}

static int linenoiseRaw(char *buf, size_t buflen, const char *prompt) {
    int count;

    if(!tk) {
        tk = termkey_new(STDIN_FILENO, 0);
        if (!atexit_registered) {
            atexit(linenoiseAtExit);
            atexit_registered = 1;
        }
    }

    if (buflen == 0) {
        errno = EINVAL;
        return -1;
    }
    if (!isatty(STDIN_FILENO)) {
        if (fgets(buf, buflen, stdin) == NULL) return -1;
        count = strlen(buf);
        if (count && buf[count-1] == '\n') {
            count--;
            buf[count] = '\0';
        }
    } else {
        if (!termkey_start(tk)) return -1;
        count = linenoisePrompt(tk, buf, buflen, prompt);
        termkey_stop(tk);
        printf("\n");
    }
    return count;
}

char *linenoise(const char *prompt) {
    char buf[LINENOISE_MAX_LINE];
    int count;

    if (isUnsupportedTerm()) {
        size_t len;

        printf("%s",prompt);
        fflush(stdout);
        if (fgets(buf,LINENOISE_MAX_LINE,stdin) == NULL) return NULL;
        len = strlen(buf);
        while(len && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
            len--;
            buf[len] = '\0';
        }
        return strdup(buf);
    } else {
        count = linenoiseRaw(buf,LINENOISE_MAX_LINE,prompt);
        if (count == -1) return NULL;
        return strdup(buf);
    }
}

/* Register a callback function to be called for tab-completion. */
void linenoiseSetCompletionCallback(linenoiseCompletionCallback *fn) {
    completionCallback = fn;
}

void linenoiseAddCompletion(linenoiseCompletions *lc, char *str) {
    size_t len = strlen(str);
    char *copy = malloc(len+1);
    memcpy(copy,str,len+1);
    lc->cvec = realloc(lc->cvec,sizeof(char*)*(lc->len+1));
    lc->cvec[lc->len++] = copy;
}

/* Using a circular buffer is smarter, but a bit more complex to handle. */
int linenoiseHistoryAdd(const char *line) {
    char *linecopy;

    if (history_max_len == 0) return 0;
    if (history == NULL) {
        history = malloc(sizeof(char*)*history_max_len);
        if (history == NULL) return 0;
        memset(history,0,(sizeof(char*)*history_max_len));
    }
    linecopy = strdup(line);
    if (!linecopy) return 0;
    if (history_len == history_max_len) {
        free(history[0]);
        memmove(history,history+1,sizeof(char*)*(history_max_len-1));
        history_len--;
    }
    history[history_len] = linecopy;
    history_len++;
    return 1;
}

int linenoiseHistorySetMaxLen(int len) {
    char **new;

    if (len < 1) return 0;
    if (history) {
        int tocopy = history_len;

        new = malloc(sizeof(char*)*len);
        if (new == NULL) return 0;
        if (len < tocopy) tocopy = len;
        memcpy(new,history+(history_max_len-tocopy), sizeof(char*)*tocopy);
        free(history);
        history = new;
    }
    history_max_len = len;
    if (history_len > history_max_len)
        history_len = history_max_len;
    return 1;
}

/* Save the history in the specified file. On success 0 is returned
 * otherwise -1 is returned. */
int linenoiseHistorySave(char *filename) {
    FILE *fp = fopen(filename,"w");
    int j;
    
    if (fp == NULL) return -1;
    for (j = 0; j < history_len; j++)
        fprintf(fp,"%s\n",history[j]);
    fclose(fp);
    return 0;
}

/* Load the history from the specified file. If the file does not exist
 * zero is returned and no operation is performed.
 *
 * If the file exists and the operation succeeded 0 is returned, otherwise
 * on error -1 is returned. */
int linenoiseHistoryLoad(char *filename) {
    FILE *fp = fopen(filename,"r");
    char buf[LINENOISE_MAX_LINE];
    
    if (fp == NULL) return -1;

    while (fgets(buf,LINENOISE_MAX_LINE,fp) != NULL) {
        char *p;
        
        p = strchr(buf,'\r');
        if (!p) p = strchr(buf,'\n');
        if (p) *p = '\0';
        linenoiseHistoryAdd(buf);
    }
    fclose(fp);
    return 0;
}
