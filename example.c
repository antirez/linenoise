#include <stdio.h>
#include <stdlib.h>
#include "linenoise.h"

int main(void) {
    char *line;

    while((line = linenoise("hello> ")) != NULL) {
        if (line[0] != '\0') {
            printf("echo: '%s'\n", line);
            linenoiseHistoryAdd(line);
        }
        free(line);
    }
    return 0;
}
