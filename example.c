#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"


void completion(const char *buf, linenoiseCompletions *lc, size_t pos) {
    const char *term[] = {"hello", "hi", "hello there"}; /* lookup terms for completion */
    const size_t n_terms = sizeof(term) / sizeof(char*); /* number of lookup terms */
    size_t i, j, n, start, termlen;
    char lookahread[1024];
    char lookback[1024];
    char newbuf[1024];

    if (pos == 0)
        return;

    /* Find the start position to compare lookup terms. */
    for (start = pos; (--start) != 0;) {
        if (start > 0 && buf[start - 1] == ' ')
            break;
    }

    /* For each lookup terms, see if it has prefix matches the input string. */
    for (n = 0; n < n_terms; n++) {
        termlen = strlen(term[n]);
        for (i = start, j = 0; i < pos && j < termlen; i++, j++) {
            if (buf[i] != term[n][j])
                break;
        }

        /* If it matches, it is a completion option to add. */
        if (i == pos) {
            /* Setup the `lookback' buffer containing string before the
             * position where lookup term being inserted. */
            snprintf(lookback, start + 1, "%s", buf);
            /* Setup the `lookahread' buffer containing string after the
             * position where lookup term being inserted. */
            strcpy(lookahread, buf + pos);
            /* Concatenate everything. */
            sprintf(newbuf, "%s%s%s", lookback, term[n], lookahread);

            /* Add this completion option. */
            linenoiseAddCompletion(lc, newbuf, start + termlen);
        }
    }
}

char *hints(const char *buf, int *color, int *bold) {
    if (!strcasecmp(buf,"hello")) {
        *color = 35;
        *bold = 0;
        return " World";
    }
    return NULL;
}

int main(int argc, char **argv) {
    char *line;
    char *prgname = argv[0];

    /* Parse options, with --multiline we enable multi line editing. */
    while(argc > 1) {
        argc--;
        argv++;
        if (!strcmp(*argv,"--multiline")) {
            linenoiseSetMultiLine(1);
            printf("Multi-line mode enabled.\n");
        } else if (!strcmp(*argv,"--keycodes")) {
            linenoisePrintKeyCodes();
            exit(0);
        } else {
            fprintf(stderr, "Usage: %s [--multiline] [--keycodes]\n", prgname);
            exit(1);
        }
    }

    /* Set the completion callback. This will be called every time the
     * user uses the <tab> key. */
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);

    /* Load history from file. The history file is just a plain text file
     * where entries are separated by newlines. */
    linenoiseHistoryLoad("history.txt"); /* Load the history at startup */

    /* Now this is the main loop of the typical linenoise-based application.
     * The call to linenoise() will block as long as the user types something
     * and presses enter.
     *
     * The typed string is returned as a malloc() allocated string by
     * linenoise, so the user needs to free() it. */
    
    while((line = linenoise("hello> ")) != NULL) {
        /* Do something with the string. */
        if (line[0] != '\0' && line[0] != '/') {
            printf("echo: '%s'\n", line);
            linenoiseHistoryAdd(line); /* Add to the history. */
            linenoiseHistorySave("history.txt"); /* Save the history on disk. */
        } else if (!strncmp(line,"/historylen",11)) {
            /* The "/historylen" command will change the history len. */
            int len = atoi(line+11);
            linenoiseHistorySetMaxLen(len);
        } else if (!strncmp(line, "/mask", 5)) {
            linenoiseMaskModeEnable();
        } else if (!strncmp(line, "/unmask", 7)) {
            linenoiseMaskModeDisable();
        } else if (line[0] == '/') {
            printf("Unreconized command: %s\n", line);
        }
        free(line);
    }
    return 0;
}
