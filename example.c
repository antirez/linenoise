#include <stdio.h>
#include <stdlib.h>
#include "linenoise.h"

int main(void) {
    char buf[1024];
    int retval;

    while(1) {
        retval = linenoise(buf,1024,"hello> ");
        if (retval > 0) {
            printf("echo: '%s'\n", buf);
            linenoiseHistoryAdd(buf);
        } else if (retval == -1) {
            exit(1);
        }
    }
    return 0;
}
