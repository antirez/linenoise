#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"


void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == 'h') {
        linenoiseAddCompletion(lc,"hello");
        linenoiseAddCompletion(lc,"hello there");
    }
}

int main(int argc, char **argv) {
    char *line;
    int i;

    for (i = 1; i < argc; i++)
        if(!strcmp("-v", argv[i])) {
            puts("vi mode on");
            linenoiseViMode(1);
        }


    linenoiseSetCompletionCallback(completion);
    linenoiseHistoryLoad("history.txt"); /* Load the history at startup */
    while((line = linenoise("hello> ")) != NULL) {
        if (line[0] != '\0') {
            printf("echo: '%s'\n", line);
            linenoiseHistoryAdd(line);
            linenoiseHistorySave("history.txt"); /* Save every new entry */
        }
        free(line);
    }
    return 0;
}
