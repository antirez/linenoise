#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"

static char buf[80];

int main(int argc, char **argv) {
    struct linenoiseInst lnInst;

    linenoiseInit(&lnInst, "hello? ");

    buf[0] = '\0';

    do {
	linenoise(&lnInst, buf, sizeof(buf));
	printf("You've entered \"%s\"\n");
    } while(strcmp(buf, "exit"));

    return 0;
}
