/* linenoise-test.c -- Test framework for linenoise with VT100 emulator.
 *
 * This file implements:
 * 1. A minimal VT100 terminal emulator that parses escape sequences
 * 2. A test harness that runs linenoise via pipes
 * 3. Visual rendering so the user can watch tests run
 * 4. Test functions and assertions
 *
 * The emulator maintains a logical screen buffer and also renders to the
 * real terminal, allowing visual verification if tests fail.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>

/* ========================= VT100 Emulator ========================= */

#define EMU_ROWS 15
#define EMU_COLS 60

/* Each screen cell stores a complete grapheme cluster and its display width.
 * Wide characters (emoji, CJK) have width=2 and occupy two cells: the main
 * cell holds the character, the next cell has width=0 (continuation).
 * Complex emoji (ZWJ sequences) can be up to ~30 bytes. */
typedef struct {
    char ch[32];  /* UTF-8 bytes for grapheme cluster + null terminator */
    int len;      /* Current length of content in ch[] */
    int width;    /* Display width: 0=continuation, 1=normal, 2=wide char */
} emu_cell_t;

static emu_cell_t emu_screen[EMU_ROWS][EMU_COLS];
static int emu_cursor_row = 0;
static int emu_cursor_col = 0;
static int emu_rows = EMU_ROWS;
static int emu_cols = EMU_COLS;
static int emu_after_zwj = 0;  /* Track if last char was ZWJ for grapheme clusters */

/* UTF-8 accumulator for multi-byte sequences. */
static char utf8_buf[5];
static int utf8_len = 0;
static int utf8_expected = 0;

/* Parser state for escape sequences. */
enum {
    STATE_NORMAL,
    STATE_ESC,      /* Saw ESC */
    STATE_CSI       /* Saw ESC [ */
};

static int parser_state = STATE_NORMAL;
static char csi_buf[32];
static int csi_len = 0;

/* Determine expected UTF-8 byte length from first byte. */
static int utf8_byte_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

/* Decode UTF-8 bytes into a codepoint. */
static uint32_t utf8_decode(const char *s, int len) {
    unsigned char c = s[0];
    uint32_t cp;
    if (len == 1) {
        cp = c;
    } else if (len == 2) {
        cp = (c & 0x1F) << 6;
        cp |= (s[1] & 0x3F);
    } else if (len == 3) {
        cp = (c & 0x0F) << 12;
        cp |= (s[1] & 0x3F) << 6;
        cp |= (s[2] & 0x3F);
    } else if (len == 4) {
        cp = (c & 0x07) << 18;
        cp |= (s[1] & 0x3F) << 12;
        cp |= (s[2] & 0x3F) << 6;
        cp |= (s[3] & 0x3F);
    } else {
        cp = c;
    }
    return cp;
}

/* Determine display width of a codepoint. Returns 0, 1 or 2. */
static int codepoint_width(uint32_t cp) {
    /* Zero-width characters. */
    if (cp == 0) return 0;
    if (cp >= 0x0300 && cp <= 0x036F) return 0;  /* Combining diacriticals */
    if (cp >= 0x1AB0 && cp <= 0x1AFF) return 0;  /* Combining diacriticals ext */
    if (cp >= 0x1DC0 && cp <= 0x1DFF) return 0;  /* Combining diacriticals sup */
    if (cp >= 0x20D0 && cp <= 0x20FF) return 0;  /* Combining for symbols */
    if (cp >= 0xFE20 && cp <= 0xFE2F) return 0;  /* Combining half marks */

    /* Grapheme-extending characters: zero width. */
    if (cp == 0xFE0E || cp == 0xFE0F) return 0;  /* Variation selectors */
    if (cp >= 0x1F3FB && cp <= 0x1F3FF) return 0; /* Skin tone modifiers */
    if (cp == 0x200D) return 0;                   /* Zero Width Joiner */

    /* Wide characters: CJK, Emoji, etc. */
    if (cp >= 0x1100 && cp <= 0x115F) return 2;  /* Hangul Jamo */
    if (cp >= 0x231A && cp <= 0x231B) return 2;  /* Watch, Hourglass */
    if (cp >= 0x23E9 && cp <= 0x23F3) return 2;  /* Various symbols */
    if (cp >= 0x23F8 && cp <= 0x23FA) return 2;  /* Various symbols */
    if (cp >= 0x25AA && cp <= 0x25AB) return 2;  /* Small squares */
    if (cp >= 0x25B6 && cp <= 0x25C0) return 2;  /* Play/reverse buttons */
    if (cp >= 0x25FB && cp <= 0x25FE) return 2;  /* Squares */
    if (cp >= 0x2600 && cp <= 0x26FF) return 2;  /* Misc symbols */
    if (cp >= 0x2700 && cp <= 0x27BF) return 2;  /* Dingbats */
    if (cp >= 0x2934 && cp <= 0x2935) return 2;  /* Arrows */
    if (cp >= 0x2B05 && cp <= 0x2B07) return 2;  /* Arrows */
    if (cp >= 0x2B1B && cp <= 0x2B1C) return 2;  /* Squares */
    if (cp == 0x2B50 || cp == 0x2B55) return 2;  /* Star, circle */
    if (cp >= 0x2E80 && cp <= 0x9FFF) return 2;  /* CJK */
    if (cp >= 0xAC00 && cp <= 0xD7AF) return 2;  /* Hangul Syllables */
    if (cp >= 0xF900 && cp <= 0xFAFF) return 2;  /* CJK Compatibility */
    if (cp >= 0xFE10 && cp <= 0xFE1F) return 2;  /* Vertical forms */
    if (cp >= 0xFE30 && cp <= 0xFE6F) return 2;  /* CJK Compatibility Forms */
    if (cp >= 0xFF00 && cp <= 0xFF60) return 2;  /* Fullwidth forms */
    if (cp >= 0xFFE0 && cp <= 0xFFE6) return 2;  /* Fullwidth symbols */
    if (cp >= 0x1F1E6 && cp <= 0x1F1FF) return 2; /* Regional indicators */
    if (cp >= 0x1F300 && cp <= 0x1F9FF) return 2; /* Emoji symbols */
    if (cp >= 0x1FA00 && cp <= 0x1FAFF) return 2; /* Emoji extended */
    if (cp >= 0x20000 && cp <= 0x2FFFF) return 2; /* CJK Extension B+ */
    if (cp >= 0x30000 && cp <= 0x3FFFF) return 2; /* CJK Extension G+ */

    return 1;
}

/* Set a cell to a space (empty). */
static void emu_clear_cell(int row, int col) {
    emu_screen[row][col].ch[0] = ' ';
    emu_screen[row][col].ch[1] = '\0';
    emu_screen[row][col].len = 1;
    emu_screen[row][col].width = 1;
}

/* Initialize the emulator. */
static void emu_init(int rows, int cols) {
    emu_rows = rows < EMU_ROWS ? rows : EMU_ROWS;
    emu_cols = cols < EMU_COLS ? cols : EMU_COLS;
    emu_cursor_row = 0;
    emu_cursor_col = 0;
    emu_after_zwj = 0;
    parser_state = STATE_NORMAL;
    csi_len = 0;
    utf8_len = 0;
    utf8_expected = 0;
    for (int r = 0; r < emu_rows; r++) {
        for (int c = 0; c < emu_cols; c++) {
            emu_clear_cell(r, c);
        }
    }
}

/* Clear from cursor to end of line. */
static void emu_clear_to_eol(void) {
    for (int c = emu_cursor_col; c < emu_cols; c++) {
        emu_clear_cell(emu_cursor_row, c);
    }
}

/* Clear entire screen. */
static void emu_clear_screen(void) {
    for (int r = 0; r < emu_rows; r++) {
        for (int c = 0; c < emu_cols; c++) {
            emu_clear_cell(r, c);
        }
    }
    emu_cursor_row = 0;
    emu_cursor_col = 0;
}

/* Parse CSI parameters (e.g., "5" from ESC[5C). */
static int csi_get_param(int def) {
    if (csi_len == 0) return def;
    csi_buf[csi_len] = '\0';
    int val = atoi(csi_buf);
    return val > 0 ? val : def;
}

/* Handle a complete CSI sequence. */
static void emu_handle_csi(char cmd) {
    int n = csi_get_param(1);

    switch (cmd) {
    case 'A':  /* Cursor Up */
        emu_cursor_row -= n;
        if (emu_cursor_row < 0) emu_cursor_row = 0;
        break;
    case 'B':  /* Cursor Down */
        emu_cursor_row += n;
        if (emu_cursor_row >= emu_rows) emu_cursor_row = emu_rows - 1;
        break;
    case 'C':  /* Cursor Forward */
        emu_cursor_col += n;
        if (emu_cursor_col >= emu_cols) emu_cursor_col = emu_cols - 1;
        break;
    case 'D':  /* Cursor Backward */
        emu_cursor_col -= n;
        if (emu_cursor_col < 0) emu_cursor_col = 0;
        break;
    case 'H':  /* Cursor Home (or position if params given) */
        emu_cursor_row = 0;
        emu_cursor_col = 0;
        break;
    case 'J':  /* Erase Display */
        if (n == 2) emu_clear_screen();
        break;
    case 'K':  /* Erase Line */
        if (n == 0 || csi_len == 0) emu_clear_to_eol();
        break;
    case 'm':  /* SGR (colors/attributes) - ignore */
        break;
    default:
        /* Unknown CSI sequence, ignore */
        break;
    }
}

/* Find the previous non-continuation cell (for appending extending chars). */
static int emu_find_prev_cell(int row, int col) {
    /* Move back to find the cell that owns this position. */
    while (col > 0) {
        col--;
        if (emu_screen[row][col].width != 0) {
            return col;
        }
    }
    return -1;  /* No previous cell found. */
}

/* Check if codepoint is Zero Width Joiner. */
static int emu_is_zwj(uint32_t cp) {
    return cp == 0x200D;
}

/* Place a complete character at the current cursor position. */
static void emu_put_char(const char *ch, int chlen) {
    uint32_t cp = utf8_decode(ch, chlen);
    int width = codepoint_width(cp);

    /* If we're after a ZWJ, append this char to the previous cell
     * regardless of its width (it's being joined). */
    if (emu_after_zwj) {
        emu_after_zwj = 0;
        int prev_col = emu_find_prev_cell(emu_cursor_row, emu_cursor_col);
        if (prev_col >= 0) {
            emu_cell_t *cell = &emu_screen[emu_cursor_row][prev_col];
            if (cell->len + chlen < (int)sizeof(cell->ch) - 1) {
                memcpy(cell->ch + cell->len, ch, chlen);
                cell->len += chlen;
                cell->ch[cell->len] = '\0';
            }
        }
        /* Check if this char is also a ZWJ (unlikely but possible). */
        if (emu_is_zwj(cp)) {
            emu_after_zwj = 1;
        }
        return;
    }

    if (width == 0) {
        /* Zero-width character - append to previous cell if possible.
         * This handles variation selectors, skin tones, ZWJ sequences. */
        int prev_col = emu_find_prev_cell(emu_cursor_row, emu_cursor_col);
        if (prev_col >= 0) {
            emu_cell_t *cell = &emu_screen[emu_cursor_row][prev_col];
            /* Append if there's room in the buffer. */
            if (cell->len + chlen < (int)sizeof(cell->ch) - 1) {
                memcpy(cell->ch + cell->len, ch, chlen);
                cell->len += chlen;
                cell->ch[cell->len] = '\0';
            }
        }
        /* If this was a ZWJ, next char should also be appended. */
        if (emu_is_zwj(cp)) {
            emu_after_zwj = 1;
        }
        return;
    }

    /* Check if there's room for this character. */
    if (emu_cursor_col + width > emu_cols) {
        /* No room, don't display (clip at edge). */
        return;
    }

    /* Before overwriting, handle orphaned continuation cells:
     * 1. If current cell is a continuation (width=0), clear it first
     * 2. If current cell was a wide char (width=2), clear its continuation */
    emu_cell_t *cur = &emu_screen[emu_cursor_row][emu_cursor_col];
    if (cur->width == 0) {
        /* This was a continuation cell - convert to space. */
        emu_clear_cell(emu_cursor_row, emu_cursor_col);
    } else if (cur->width == 2 && emu_cursor_col + 1 < emu_cols) {
        /* This was a wide char - clear its orphaned continuation. */
        emu_clear_cell(emu_cursor_row, emu_cursor_col + 1);
    }

    /* Store the character in the current cell. */
    memcpy(emu_screen[emu_cursor_row][emu_cursor_col].ch, ch, chlen);
    emu_screen[emu_cursor_row][emu_cursor_col].ch[chlen] = '\0';
    emu_screen[emu_cursor_row][emu_cursor_col].len = chlen;
    emu_screen[emu_cursor_row][emu_cursor_col].width = width;
    emu_cursor_col++;

    /* For wide characters, mark the next cell as continuation. */
    if (width == 2 && emu_cursor_col < emu_cols) {
        emu_screen[emu_cursor_row][emu_cursor_col].ch[0] = '\0';
        emu_screen[emu_cursor_row][emu_cursor_col].len = 0;
        emu_screen[emu_cursor_row][emu_cursor_col].width = 0;
        emu_cursor_col++;
    }
}

/* Feed a single byte to the emulator. */
static void emu_feed_byte(unsigned char c) {
    switch (parser_state) {
    case STATE_NORMAL:
        if (c == 0x1b) {
            parser_state = STATE_ESC;
            utf8_len = 0;  /* Cancel any pending UTF-8 sequence. */
        } else if (c == '\r') {
            emu_cursor_col = 0;
            utf8_len = 0;
        } else if (c == '\n') {
            emu_cursor_row++;
            if (emu_cursor_row >= emu_rows) {
                /* Scroll up: move all rows up, clear bottom row. */
                for (int r = 0; r < emu_rows - 1; r++) {
                    memcpy(emu_screen[r], emu_screen[r + 1],
                           sizeof(emu_cell_t) * emu_cols);
                }
                for (int c2 = 0; c2 < emu_cols; c2++) {
                    emu_clear_cell(emu_rows - 1, c2);
                }
                emu_cursor_row = emu_rows - 1;
            }
            utf8_len = 0;
        } else if (c == '\b') {
            if (emu_cursor_col > 0) {
                emu_cursor_col--;
                /* If we're on a continuation cell, back up one more. */
                if (emu_screen[emu_cursor_row][emu_cursor_col].width == 0 &&
                    emu_cursor_col > 0) {
                    emu_cursor_col--;
                }
            }
            utf8_len = 0;
        } else if (c >= 32 || (c & 0x80)) {
            /* Printable character or UTF-8 byte. */
            if ((c & 0x80) == 0) {
                /* ASCII character - display immediately. */
                char ch[2] = {c, '\0'};
                emu_put_char(ch, 1);
                utf8_len = 0;
            } else if ((c & 0xC0) == 0xC0) {
                /* Start of UTF-8 multi-byte sequence. */
                utf8_buf[0] = c;
                utf8_len = 1;
                utf8_expected = utf8_byte_len(c);
            } else if ((c & 0xC0) == 0x80 && utf8_len > 0) {
                /* Continuation byte. */
                utf8_buf[utf8_len++] = c;
                if (utf8_len >= utf8_expected) {
                    /* Complete UTF-8 character. */
                    utf8_buf[utf8_len] = '\0';
                    emu_put_char(utf8_buf, utf8_len);
                    utf8_len = 0;
                }
            } else {
                /* Invalid UTF-8 - reset. */
                utf8_len = 0;
            }
        }
        break;

    case STATE_ESC:
        if (c == '[') {
            parser_state = STATE_CSI;
            csi_len = 0;
        } else {
            /* Unknown escape, back to normal. */
            parser_state = STATE_NORMAL;
        }
        break;

    case STATE_CSI:
        if (c >= '0' && c <= '9') {
            if (csi_len < (int)sizeof(csi_buf) - 1) {
                csi_buf[csi_len++] = c;
            }
        } else if (c == ';') {
            /* Multiple params - for simplicity, just reset. */
            csi_len = 0;
        } else {
            /* End of CSI sequence. */
            emu_handle_csi(c);
            parser_state = STATE_NORMAL;
        }
        break;
    }
}

/* Debug flag for verbose output. */
static int emu_debug = 0;

/* Feed a buffer to the emulator. */
static void emu_feed(const char *buf, int len) {
    if (emu_debug) {
        printf("EMU_FEED (%d bytes): ", len);
        for (int i = 0; i < len && i < 200; i++) {
            unsigned char c = buf[i];
            if (c >= 32 && c < 127) printf("%c", c);
            else printf("<%02X>", c);
        }
        if (len > 200) printf("...");
        printf("\n");
    }
    for (int i = 0; i < len; i++) {
        emu_feed_byte((unsigned char)buf[i]);
    }
}

/* Get a row from the screen as a UTF-8 string (trimmed of trailing spaces). */
static const char *emu_get_row(int row) {
    static char buf[EMU_COLS * 4 + 1];  /* Each cell can be up to 4 UTF-8 bytes. */
    if (row < 0 || row >= emu_rows) {
        buf[0] = '\0';
        return buf;
    }
    /* Build the row string, skipping continuation cells. */
    int pos = 0;
    int last_non_space = -1;
    for (int c = 0; c < emu_cols; c++) {
        emu_cell_t *cell = &emu_screen[row][c];
        if (cell->width == 0) continue;  /* Skip continuation cells. */

        int chlen = strlen(cell->ch);
        if (pos + chlen < (int)sizeof(buf) - 1) {
            memcpy(buf + pos, cell->ch, chlen);
            if (!(chlen == 1 && cell->ch[0] == ' ')) {
                last_non_space = pos + chlen;
            }
            pos += chlen;
        }
    }
    /* Trim trailing spaces. */
    if (last_non_space >= 0) {
        buf[last_non_space] = '\0';
    } else {
        buf[0] = '\0';
    }
    return buf;
}

/* ========================= Visual Rendering ========================= */

/* Render the emulator state to the real terminal for visual inspection.
 * This shows the screen contents and cursor position. */
static void render_to_terminal(const char *test_name) {
    /* Clear real screen and move home. */
    printf("\x1b[2J\x1b[H");

    /* Header. */
    printf("\x1b[1;36m=== LINENOISE TEST: %s ===\x1b[0m\n\n", test_name);

    /* Draw screen with border. */
    printf("\x1b[33m+");
    for (int c = 0; c < emu_cols; c++) printf("-");
    printf("+\x1b[0m\n");

    for (int r = 0; r < emu_rows; r++) {
        printf("\x1b[33m|\x1b[0m");
        for (int c = 0; c < emu_cols; c++) {
            emu_cell_t *cell = &emu_screen[r][c];

            if (cell->width == 0) {
                /* Continuation cell - skip (already printed with wide char). */
                continue;
            }

            if (r == emu_cursor_row && c == emu_cursor_col) {
                /* Highlight cursor position. */
                printf("\x1b[7m%s\x1b[0m", cell->ch);
            } else {
                printf("%s", cell->ch);
            }
        }
        printf("\x1b[33m|\x1b[0m\n");
    }

    printf("\x1b[33m+");
    for (int c = 0; c < emu_cols; c++) printf("-");
    printf("+\x1b[0m\n");

    /* Cursor info. */
    printf("\nCursor: row=%d, col=%d\n", emu_cursor_row, emu_cursor_col);
    fflush(stdout);
}

/* ========================= Test Harness ========================= */

static int child_pid = -1;
static int pipe_to_child[2];    /* We write, child reads (child's stdin) */
static int pipe_from_child[2];  /* Child writes, we read (child's stdout) */
static const char *current_test = "unknown";

/* Start the linenoise example program. */
static int test_start(const char *test_name, const char *program) {
    current_test = test_name;
    emu_init(EMU_ROWS, EMU_COLS);

    if (pipe(pipe_to_child) == -1) {
        perror("pipe");
        return -1;
    }
    if (pipe(pipe_from_child) == -1) {
        perror("pipe");
        return -1;
    }

    child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        return -1;
    }

    if (child_pid == 0) {
        /* Child process. */
        close(pipe_to_child[1]);   /* Close write end. */
        close(pipe_from_child[0]); /* Close read end. */

        dup2(pipe_to_child[0], STDIN_FILENO);
        dup2(pipe_from_child[1], STDOUT_FILENO);
        dup2(pipe_from_child[1], STDERR_FILENO);

        close(pipe_to_child[0]);
        close(pipe_from_child[1]);

        /* Set test environment variables. */
        setenv("LINENOISE_ASSUME_TTY", "1", 1);
        setenv("LINENOISE_COLS", "60", 1);

        /* Use shell to parse the command line arguments. */
        execl("/bin/sh", "sh", "-c", program, NULL);
        perror("exec");
        exit(1);
    }

    /* Parent process. */
    close(pipe_to_child[0]);   /* Close read end. */
    close(pipe_from_child[1]); /* Close write end. */

    /* Give child time to start and print prompt. */
    usleep(50000);  /* 50ms */

    /* Read initial output (prompt) with timeout. */
    char buf[4096];
    fd_set fds;
    struct timeval tv = {1, 0};  /* 1 second timeout */

    FD_ZERO(&fds);
    FD_SET(pipe_from_child[0], &fds);

    if (select(pipe_from_child[0] + 1, &fds, NULL, NULL, &tv) > 0) {
        int n = read(pipe_from_child[0], buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            emu_feed(buf, n);
        }
    }

    render_to_terminal(test_name);
    return 0;
}

/* End the test, clean up. */
static void test_end(void) {
    if (child_pid > 0) {
        /* Send Ctrl-D (EOF) to terminate cleanly. */
        write(pipe_to_child[1], "\x04", 1);
        usleep(50000);

        /* Close our end of the pipe to signal EOF. */
        close(pipe_to_child[1]);

        /* Wait briefly for child to exit. */
        int status;
        int wait_result = waitpid(child_pid, &status, WNOHANG);
        if (wait_result == 0) {
            /* Child didn't exit, send SIGTERM. */
            kill(child_pid, SIGTERM);
            usleep(10000);
            waitpid(child_pid, &status, WNOHANG);
        }
        child_pid = -1;
    } else {
        close(pipe_to_child[1]);
    }
    close(pipe_from_child[0]);
}

/* Send keys to linenoise and read response. */
static void send_keys(const char *keys) {
    write(pipe_to_child[1], keys, strlen(keys));
    usleep(30000);  /* 30ms - give linenoise time to process. */

    /* Read response with timeout. */
    char buf[4096];
    fd_set fds;
    struct timeval tv;
    int max_reads = 10;  /* Prevent infinite loop. */

    while (max_reads-- > 0) {
        FD_ZERO(&fds);
        FD_SET(pipe_from_child[0], &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 50000;  /* 50ms timeout */

        if (select(pipe_from_child[0] + 1, &fds, NULL, NULL, &tv) <= 0) {
            break;  /* Timeout or error. */
        }
        int n = read(pipe_from_child[0], buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[n] = '\0';
        emu_feed(buf, n);
    }

    render_to_terminal(current_test);
}

/* Send special keys. */
#define KEY_UP      "\x1b[A"
#define KEY_DOWN    "\x1b[B"
#define KEY_RIGHT   "\x1b[C"
#define KEY_LEFT    "\x1b[D"
#define KEY_HOME    "\x1b[H"
#define KEY_END     "\x1b[F"
#define KEY_DELETE  "\x1b[3~"
#define KEY_BACKSPACE "\x7f"
#define KEY_ENTER   "\r"
#define KEY_CTRL_A  "\x01"
#define KEY_CTRL_E  "\x05"
#define KEY_CTRL_U  "\x15"
#define KEY_CTRL_K  "\x0b"
#define KEY_CTRL_W  "\x17"
#define KEY_CTRL_T  "\x14"
#define KEY_CTRL_C  "\x03"

/* ========================= Test Assertions ========================= */

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

static void assert_screen_row(int row, const char *expected) {
    tests_run++;
    const char *actual = emu_get_row(row);
    if (strcmp(actual, expected) == 0) {
        tests_passed++;
        printf("\x1b[32m[PASS]\x1b[0m Row %d == \"%s\"\n", row, expected);
    } else {
        tests_failed++;
        printf("\x1b[31m[FAIL]\x1b[0m Row %d:\n", row);
        printf("       Expected: \"%s\"\n", expected);
        printf("       Actual:   \"%s\"\n", actual);
    }
    fflush(stdout);
}

static void assert_cursor(int row, int col) {
    tests_run++;
    if (emu_cursor_row == row && emu_cursor_col == col) {
        tests_passed++;
        printf("\x1b[32m[PASS]\x1b[0m Cursor at (%d, %d)\n", row, col);
    } else {
        tests_failed++;
        printf("\x1b[31m[FAIL]\x1b[0m Cursor position:\n");
        printf("       Expected: (%d, %d)\n", row, col);
        printf("       Actual:   (%d, %d)\n", emu_cursor_row, emu_cursor_col);
    }
    fflush(stdout);
}

static void assert_row_contains(int row, const char *substr) {
    tests_run++;
    const char *actual = emu_get_row(row);
    if (strstr(actual, substr) != NULL) {
        tests_passed++;
        printf("\x1b[32m[PASS]\x1b[0m Row %d contains \"%s\"\n", row, substr);
    } else {
        tests_failed++;
        printf("\x1b[31m[FAIL]\x1b[0m Row %d doesn't contain \"%s\"\n", row, substr);
        printf("       Actual: \"%s\"\n", actual);
    }
    fflush(stdout);
}

/* Assert that a cell contains specific bytes (for verifying grapheme clusters). */
static void assert_cell_content(int row, int col, const char *expected, int expected_len) {
    tests_run++;
    emu_cell_t *cell = &emu_screen[row][col];
    if (cell->len == expected_len && memcmp(cell->ch, expected, expected_len) == 0) {
        tests_passed++;
        printf("\x1b[32m[PASS]\x1b[0m Cell (%d,%d) contains %d bytes\n", row, col, expected_len);
    } else {
        tests_failed++;
        printf("\x1b[31m[FAIL]\x1b[0m Cell (%d,%d) content mismatch:\n", row, col);
        printf("       Expected: %d bytes [", expected_len);
        for (int i = 0; i < expected_len; i++) printf("%02X ", (unsigned char)expected[i]);
        printf("]\n");
        printf("       Actual:   %d bytes [", cell->len);
        for (int i = 0; i < cell->len; i++) printf("%02X ", (unsigned char)cell->ch[i]);
        printf("]\n");
    }
    fflush(stdout);
}

/* Assert that a cell has the expected display width. */
static void assert_cell_width(int row, int col, int expected_width) {
    tests_run++;
    emu_cell_t *cell = &emu_screen[row][col];
    if (cell->width == expected_width) {
        tests_passed++;
        printf("\x1b[32m[PASS]\x1b[0m Cell (%d,%d) width == %d\n", row, col, expected_width);
    } else {
        tests_failed++;
        printf("\x1b[31m[FAIL]\x1b[0m Cell (%d,%d) width:\n", row, col);
        printf("       Expected: %d\n", expected_width);
        printf("       Actual:   %d\n", cell->width);
    }
    fflush(stdout);
}

/* ========================= Tests ========================= */

static void test_simple_typing(void) {
    if (test_start("Simple Typing", "./linenoise-example") == -1) return;

    send_keys("hello");
    assert_row_contains(0, "hello");
    assert_cursor(0, strlen("hello> ") + 5);

    send_keys(" world");
    assert_screen_row(0, "hello> hello world");

    test_end();
}

static void test_cursor_movement(void) {
    if (test_start("Cursor Movement", "./linenoise-example") == -1) return;

    send_keys("abcdef");
    int prompt_len = strlen("hello> ");

    /* Move left 3 times. */
    send_keys(KEY_LEFT KEY_LEFT KEY_LEFT);
    assert_cursor(0, prompt_len + 3);  /* After "abc" */

    /* Move right 1 time. */
    send_keys(KEY_RIGHT);
    assert_cursor(0, prompt_len + 4);  /* After "abcd" */

    /* Home. */
    send_keys(KEY_CTRL_A);
    assert_cursor(0, prompt_len);

    /* End. */
    send_keys(KEY_CTRL_E);
    assert_cursor(0, prompt_len + 6);

    test_end();
}

static void test_backspace_delete(void) {
    if (test_start("Backspace and Delete", "./linenoise-example") == -1) return;

    send_keys("hello");
    int prompt_len = strlen("hello> ");

    /* Backspace. */
    send_keys(KEY_BACKSPACE);
    assert_row_contains(0, "hell");
    assert_cursor(0, prompt_len + 4);

    /* Move left and delete forward. */
    send_keys(KEY_LEFT KEY_LEFT);
    send_keys(KEY_DELETE);
    assert_row_contains(0, "hel");

    test_end();
}

static void test_utf8_typing(void) {
    if (test_start("UTF-8 Typing", "./linenoise-example") == -1) return;

    /* Type some UTF-8 characters. */
    send_keys("caf\xc3\xa9");  /* "café" - é is 2 bytes */
    assert_row_contains(0, "café");

    test_end();
}

static void test_utf8_emoji(void) {
    if (test_start("UTF-8 Emoji", "./linenoise-example") == -1) return;

    int prompt_len = strlen("hello> ");

    /* Type text with emoji (🎉 is 4 bytes, displays as 2 columns). */
    send_keys("hi \xf0\x9f\x8e\x89 there");  /* "hi 🎉 there" */
    assert_row_contains(0, "hi");

    /* The emoji takes 2 columns, so cursor should be at:
     * prompt(7) + "hi "(3) + emoji(2) + " there"(6) = 18 */
    assert_cursor(0, prompt_len + 3 + 2 + 6);

    test_end();
}

static void test_utf8_cursor_over_emoji(void) {
    if (test_start("UTF-8 Cursor Over Emoji", "./linenoise-example") == -1) return;

    int prompt_len = strlen("hello> ");

    /* Type: "a🎉b" */
    send_keys("a\xf0\x9f\x8e\x89" "b");
    /* Cursor after 'b': prompt + 'a'(1) + emoji(2) + 'b'(1) = prompt + 4 */
    assert_cursor(0, prompt_len + 4);

    /* Move left over 'b'. */
    send_keys(KEY_LEFT);
    assert_cursor(0, prompt_len + 3);  /* After emoji */

    /* Move left over emoji (should move 2 columns in one keystroke). */
    send_keys(KEY_LEFT);
    assert_cursor(0, prompt_len + 1);  /* After 'a' */

    /* Move left over 'a'. */
    send_keys(KEY_LEFT);
    assert_cursor(0, prompt_len);  /* At start */

    test_end();
}

static void test_utf8_backspace_emoji(void) {
    if (test_start("UTF-8 Backspace Emoji", "./linenoise-example") == -1) return;

    /* Type: "x🎉y" then backspace should delete 'y', then emoji, then 'x'. */
    send_keys("x\xf0\x9f\x8e\x89" "y");
    assert_row_contains(0, "x");  /* Contains at least 'x' */

    send_keys(KEY_BACKSPACE);  /* Delete 'y' */
    /* Now should be "x🎉" */

    send_keys(KEY_BACKSPACE);  /* Delete emoji (4 bytes, one backspace) */
    assert_row_contains(0, "hello> x");

    send_keys(KEY_BACKSPACE);  /* Delete 'x' */
    /* Now should be empty after prompt */

    /* Type new text to verify buffer is truly empty (no orphaned bytes). */
    send_keys("ok");
    assert_row_contains(0, "hello> ok");

    test_end();
}

static void test_utf8_backspace_4byte_only(void) {
    if (test_start("UTF-8 Backspace 4-byte Only", "./linenoise-example") == -1) return;

    int prompt_len = strlen("hello> ");

    /* Type a single 4-byte emoji (robot 🤖 = F0 9F A4 96). */
    send_keys("\xf0\x9f\xa4\x96");
    assert_cursor(0, prompt_len + 2);  /* Emoji is 2 columns wide */

    /* Backspace should delete the entire 4-byte emoji in one keystroke. */
    send_keys(KEY_BACKSPACE);
    assert_cursor(0, prompt_len);  /* Cursor should be at prompt end */

    /* Type new text to verify no orphaned bytes remain in buffer. */
    send_keys("test");
    assert_row_contains(0, "hello> test");

    /* The row should NOT contain any garbage characters. */
    /* If there were orphaned bytes, "test" would appear after them. */

    test_end();
}

static void test_utf8_grapheme_clusters(void) {
    if (test_start("UTF-8 Grapheme Clusters", "./linenoise-example") == -1) return;

    int prompt_len = strlen("hello> ");

    /* Test 1: Heart with variation selector ❤️ (U+2764 + U+FE0F = 6 bytes).
     * Bytes: E2 9D A4 EF B8 8F */
    send_keys("\xe2\x9d\xa4\xef\xb8\x8f");
    assert_cursor(0, prompt_len + 2);  /* Emoji is 2 columns wide */

    /* Backspace should delete the entire grapheme cluster (6 bytes). */
    send_keys(KEY_BACKSPACE);
    assert_cursor(0, prompt_len);

    /* Verify buffer is clean by typing new text. */
    send_keys("a");
    assert_row_contains(0, "hello> a");
    send_keys(KEY_BACKSPACE);

    /* Test 2: Thumbs up with skin tone 👍🏻 (U+1F44D + U+1F3FB = 8 bytes).
     * Bytes: F0 9F 91 8D F0 9F 8F BB */
    send_keys("\xf0\x9f\x91\x8d\xf0\x9f\x8f\xbb");
    assert_cursor(0, prompt_len + 2);  /* Still 2 columns (skin tone is zero-width) */

    /* Backspace should delete the entire grapheme cluster (8 bytes). */
    send_keys(KEY_BACKSPACE);
    assert_cursor(0, prompt_len);

    /* Verify buffer is clean. */
    send_keys("b");
    assert_row_contains(0, "hello> b");
    send_keys(KEY_BACKSPACE);

    /* Test 3: Rainbow flag 🏳️‍🌈 (U+1F3F3 + U+FE0F + U+200D + U+1F308 = 14 bytes).
     * Bytes: F0 9F 8F B3 EF B8 8F E2 80 8D F0 9F 8C 88 */
    send_keys("\xf0\x9f\x8f\xb3\xef\xb8\x8f\xe2\x80\x8d\xf0\x9f\x8c\x88");
    /* This should render as 2 columns (single emoji).
     * The ZWJ-joined rainbow should not add extra width. */
    assert_cursor(0, prompt_len + 2);

    /* Backspace should delete the entire ZWJ sequence. */
    send_keys(KEY_BACKSPACE);
    assert_cursor(0, prompt_len);

    /* Verify buffer is clean. */
    send_keys("c");
    assert_row_contains(0, "hello> c");
    send_keys(KEY_BACKSPACE);

    /* Test 4: Family emoji 👨‍👩‍👧 (man + ZWJ + woman + ZWJ + girl = 18 bytes).
     * Bytes: F0 9F 91 A8 E2 80 8D F0 9F 91 A9 E2 80 8D F0 9F 91 A7
     * Should render as 2 columns despite having 3 emoji joined by ZWJ. */
    send_keys("\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x91\xa7");
    assert_cursor(0, prompt_len + 2);

    /* Backspace should delete the entire ZWJ sequence. */
    send_keys(KEY_BACKSPACE);
    assert_cursor(0, prompt_len);

    /* Verify buffer is clean. */
    send_keys("ok");
    assert_row_contains(0, "hello> ok");

    test_end();
}

static void test_utf8_grapheme_cursor_movement(void) {
    if (test_start("UTF-8 Grapheme Cursor Movement", "./linenoise-example") == -1) return;

    int prompt_len = strlen("hello> ");

    /* Type: a + thumbs up with skin tone + b
     * 👍🏻 = F0 9F 91 8D F0 9F 8F BB (8 bytes, 2 columns)
     * Layout: prompt(7) + a(1) + 👍🏻(2) + b(1) = 11 total columns */
    send_keys("a\xf0\x9f\x91\x8d\xf0\x9f\x8f\xbb" "b");
    assert_cursor(0, prompt_len + 4);  /* 7 + 1 + 2 + 1 = 11 */

    /* Move left over 'b'. */
    send_keys(KEY_LEFT);
    assert_cursor(0, prompt_len + 3);  /* 7 + 1 + 2 = 10 */

    /* Move left over thumbs up (should move 2 columns in one keystroke). */
    send_keys(KEY_LEFT);
    assert_cursor(0, prompt_len + 1);  /* 7 + 1 = 8 */

    /* Move left over 'a'. */
    send_keys(KEY_LEFT);
    assert_cursor(0, prompt_len);  /* 7 */

    /* Move right over 'a'. */
    send_keys(KEY_RIGHT);
    assert_cursor(0, prompt_len + 1);  /* 7 + 1 = 8 */

    /* Move right over thumbs up (should move 2 columns in one keystroke). */
    send_keys(KEY_RIGHT);
    assert_cursor(0, prompt_len + 3);  /* 7 + 1 + 2 = 10 */

    /* Move right over 'b'. */
    send_keys(KEY_RIGHT);
    assert_cursor(0, prompt_len + 4);  /* 7 + 1 + 2 + 1 = 11 */

    test_end();
}

static void test_emulator_grapheme_storage(void) {
    if (test_start("Emulator Grapheme Storage", "./linenoise-example") == -1) return;
    emu_debug = 1;  /* Enable debug output */

    int prompt_len = strlen("hello> ");

    /* Test 1: Thumbs up with skin tone 👍🏻 should be stored as single cell.
     * U+1F44D + U+1F3FB = 8 bytes.
     * Bytes: F0 9F 91 8D F0 9F 8F BB */
    const char thumbs_up[] = "\xf0\x9f\x91\x8d\xf0\x9f\x8f\xbb";
    send_keys(thumbs_up);

    /* Cell at column 7 should contain all 8 bytes. */
    assert_cell_content(0, prompt_len, thumbs_up, 8);
    assert_cell_width(0, prompt_len, 2);

    /* Cell at column 8 should be continuation (width=0). */
    assert_cell_width(0, prompt_len + 1, 0);

    send_keys(KEY_BACKSPACE);

    /* Test 2: Heart with variation selector ❤️ should be stored as single cell.
     * U+2764 + U+FE0F = 6 bytes.
     * Bytes: E2 9D A4 EF B8 8F */
    const char heart[] = "\xe2\x9d\xa4\xef\xb8\x8f";
    send_keys(heart);

    /* Cell at column 7 should contain all 6 bytes. */
    assert_cell_content(0, prompt_len, heart, 6);
    assert_cell_width(0, prompt_len, 2);

    /* Cell at column 8 should be continuation. */
    assert_cell_width(0, prompt_len + 1, 0);

    test_end();
}

static void test_ctrl_w_delete_word(void) {
    if (test_start("Ctrl-W Delete Word", "./linenoise-example") == -1) return;

    send_keys("hello world");
    send_keys(KEY_CTRL_W);  /* Delete "world" */
    assert_row_contains(0, "hello ");

    send_keys(KEY_CTRL_W);  /* Delete "hello " */
    /* Should be empty now. */

    test_end();
}

static void test_ctrl_u_delete_line(void) {
    if (test_start("Ctrl-U Delete Line", "./linenoise-example") == -1) return;

    int prompt_len = strlen("hello> ");

    send_keys("hello world");
    send_keys(KEY_CTRL_U);  /* Delete entire line */
    assert_cursor(0, prompt_len);  /* Cursor should be at start of input */

    /* Type new text to verify buffer was cleared. */
    send_keys("new");
    assert_row_contains(0, "hello> new");

    test_end();
}

static void test_horizontal_scroll(void) {
    if (test_start("Horizontal Scroll", "./linenoise-example") == -1) return;

    int prompt_len = strlen("hello> ");  /* 7 chars */

    /* Type text longer than the line (70 chars). The display should scroll
     * horizontally to keep the cursor visible.
     * prompt(7) + 70 = 77 > 60, causes scrolling. */
    send_keys("aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee"
              "ffffffffffffffffffff");  /* 70 chars: 50 + 20 */

    /* The right side of the text should be visible (scrolled left).
     * Cursor should be at the right edge. */
    assert_cursor(0, 59);  /* At last column (60-col terminal) */
    assert_row_contains(0, "ffffffffffffffffffff");  /* The end should be visible */

    /* Move cursor to beginning - text should scroll to show start. */
    send_keys(KEY_CTRL_A);
    assert_cursor(0, prompt_len);  /* After prompt */
    assert_row_contains(0, "hello> aaaaaaaaaa");  /* Start should now be visible */

    /* Move cursor to end - text should scroll back. */
    send_keys(KEY_CTRL_E);
    assert_cursor(0, 59);
    assert_row_contains(0, "ffffffffffffffffffff");  /* End visible again */

    /* Delete some chars from the end and verify left portion reappears. */
    for (int i = 0; i < 20; i++) send_keys(KEY_BACKSPACE);  /* Delete 20 chars */

    /* Now 50 chars remain, which fits: prompt(7) + 50 = 57 < 60 */
    assert_row_contains(0, "hello> aaaaaaaaaa");  /* Start should be visible */
    assert_row_contains(0, "eeeeeeeeee");  /* And most of the text */

    test_end();
}

static void test_horizontal_scroll_utf8(void) {
    if (test_start("Horizontal Scroll UTF-8", "./linenoise-example") == -1) return;

    int prompt_len = strlen("hello> ");  /* 7 cols */

    /* Type text with emojis that fills most of the line.
     * Each emoji is 4 bytes but 2 columns.
     * Type: "START" (5 cols) + 20 emojis (40 cols) + "END" (3 cols) = 48 cols.
     * With prompt (7 cols), total = 55 cols, fits in 60-col terminal. */
    send_keys("START");
    /* Send 20 emojis in one batch. */
    send_keys("\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89"
              "\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89"
              "\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89"
              "\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89\xf0\x9f\x8e\x89");
    send_keys("END");

    /* Verify both START and END are visible (line fits). */
    assert_row_contains(0, "START");
    assert_row_contains(0, "END");

    /* Move to start and verify cursor position. */
    send_keys(KEY_CTRL_A);
    assert_cursor(0, prompt_len);

    /* Insert at beginning and verify. */
    send_keys("X");
    assert_row_contains(0, "hello> XSTART");

    test_end();
}

/* ========================= Multi-line Mode Tests ========================= */

static void test_multiline_wrap(void) {
    if (test_start("Multiline Wrap", "./linenoise-example --multiline") == -1) return;

    /* Type a line longer than 60 cols to force wrapping.
     * Prompt is 7 chars ("hello> "), so we need 54+ chars to wrap. */
    send_keys("aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee"
              "ffffffffff");  /* 60 chars */

    /* In multiline mode, full content should be displayed across rows.
     * Just verify the content is there (not clipped like single-line mode). */
    assert_row_contains(0, "hello> aaaaaaaaaa");

    test_end();
}

static void test_multiline_cursor_movement(void) {
    if (test_start("Multiline Cursor Movement", "./linenoise-example --multiline") == -1) return;

    /* Type text that wraps (60 chars wraps on 60-col terminal). */
    send_keys("aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee"
              "ffffffffff");  /* 60 chars */

    /* Move to beginning (Ctrl-A). */
    send_keys(KEY_CTRL_A);
    /* Type something at the beginning to verify cursor position. */
    send_keys("X");
    assert_row_contains(0, "hello> Xaaaaaaaaaa");  /* X inserted at start */

    /* Move to end (Ctrl-E) and type. */
    send_keys(KEY_CTRL_E);
    send_keys("Z");
    /* The 'Z' should be at the end. We can't easily verify row position,
     * but content should be updated. */

    test_end();
}

static void test_multiline_utf8(void) {
    if (test_start("Multiline UTF-8", "./linenoise-example --multiline") == -1) return;

    /* Type text with emoji. Each emoji is 4 bytes, 2 cols. */
    send_keys("Test ");
    for (int i = 0; i < 10; i++) {
        send_keys("\xf0\x9f\x8e\x89");  /* 🎉 - 4 bytes, 2 cols */
    }
    /* 7 (prompt) + 5 ("Test ") + 20 (10 emojis * 2 cols) = 32 cols, fits on one line */

    assert_row_contains(0, "Test");

    /* Backspace should delete one emoji (4 bytes) at a time. */
    send_keys(KEY_BACKSPACE);
    /* Now 9 emojis remain. */

    /* Move to start and insert more. */
    send_keys(KEY_CTRL_A);
    send_keys("Hi ");
    assert_row_contains(0, "hello> Hi Test");

    test_end();
}

static void test_multiline_history(void) {
    if (test_start("Multiline History Navigation", "./linenoise-example --multiline") == -1) return;

    /* Type a long line that wraps to 2 rows.
     * Prompt is 7 chars ("hello> "), so we need 54+ chars to wrap on 60-col terminal. */
    send_keys("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    /* This is 64 chars, with 7 char prompt = 71 cols, wraps to 2 rows on 60-col terminal. */

    /* Press Enter to commit to history. */
    send_keys(KEY_ENTER);

    /* Now we have a new prompt. Type a short line. */
    send_keys("short");
    assert_row_contains(0, "hello> short");

    /* Press Enter to commit the short line to history. */
    send_keys(KEY_ENTER);

    /* Navigate UP to get the short line from history. */
    send_keys(KEY_UP);
    assert_row_contains(0, "hello> short");

    /* Navigate UP again to get the long line. */
    send_keys(KEY_UP);
    /* The long line wraps, check first row. */
    assert_row_contains(0, "hello> aaaaaa");

    /* Navigate DOWN to go back to short line.
     * This is the critical test: the long line should be fully cleared
     * and only the short line should remain visible. */
    send_keys(KEY_DOWN);
    assert_row_contains(0, "hello> short");

    /* Verify row 1 is empty (no leftover from the long line). */
    assert_screen_row(1, "");

    test_end();
}

/* ========================= Main ========================= */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("\x1b[2J\x1b[H");  /* Clear screen */
    printf("\x1b[1;35m");
    printf("╔════════════════════════════════════════╗\n");
    printf("║     LINENOISE TEST SUITE               ║\n");
    printf("║     With VT100 Emulator                ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\x1b[0m\n");

    /* Run single-line mode tests. */
    test_simple_typing();
    test_cursor_movement();
    test_backspace_delete();
    test_utf8_typing();
    test_utf8_emoji();
    test_utf8_cursor_over_emoji();
    test_utf8_backspace_emoji();
    test_utf8_backspace_4byte_only();
    test_utf8_grapheme_clusters();
    test_utf8_grapheme_cursor_movement();
    test_emulator_grapheme_storage();
    test_ctrl_w_delete_word();
    test_ctrl_u_delete_line();

    /* Horizontal scrolling tests (single-line mode). */
    test_horizontal_scroll();
    test_horizontal_scroll_utf8();

    /* Run multi-line mode tests. */
    test_multiline_wrap();
    test_multiline_cursor_movement();
    test_multiline_utf8();
    test_multiline_history();

    /* Summary. */
    printf("\n\x1b[1;35m");
    printf("╔════════════════════════════════════════╗\n");
    printf("║     TEST RESULTS                       ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\x1b[0m\n");

    printf("Tests run:    %d\n", tests_run);
    printf("\x1b[32mTests passed: %d\x1b[0m\n", tests_passed);
    if (tests_failed > 0) {
        printf("\x1b[31mTests failed: %d\x1b[0m\n", tests_failed);
    } else {
        printf("Tests failed: %d\n", tests_failed);
    }

    return tests_failed > 0 ? 1 : 0;
}
