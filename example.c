
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"

void completion(const char *buf, linenoiseCompletions *lc) {

  if (buf[0] != 'h') return;

  linenoiseAddCompletion(lc, "hello");
  linenoiseAddCompletion(lc, "hello there");
}

int main(int argc, char **argv) {

  char *line, *prgname = argv[0]; printf("arg0: %s\n", argv[0]);

  while (argc > 1) { /// Parse options, with --multiline we enable multi line editing.

    argc--; argv++;

      !strcmp(*argv, "--multiline") ? linenoiseSetMultiLine(1), printf("Multi-line mode enabled.\n")
    : !strcmp(*argv,  "--keycodes") ? linenoisePrintKeyCodes(), exit(0)
    : fprintf(stderr, "Usage: %s [--multiline] [--keycodes]\n", prgname), exit(1);
  }

  /// Set the completion callback. This will be called every time the user uses the <tab> key.

  linenoiseSetCompletionCallback(completion);

  /// Load history from file. The history file is just a plain text file where entries are separated by newlines.

  linenoiseHistoryLoad("/usr/share/dict/propernames"); /// Load the history at startup

  /*! Now, THIS is the 'main' loop of the typical linenoise-based application.
    The call to linenoise() will block as long as the user types something and presses enter.
    The typed string is returned as a malloc() allocated string by linenoise, so the user needs to free() it. */

  while ((line = linenoise("linenoise > "))) {

    line[0] != '\0' && line[0] != '/' ? ({      /// Do something with the string.

          printf("echo: '%s'\n", line);
          linenoiseHistoryAdd(line);            /// Add to the history.
          linenoiseHistorySave("history.txt");  /// Save the history on disk.

    }) : !strncmp(line, "/historylen", 11)      /// The "/historylen" command will change the history len.

       ? linenoiseHistorySetMaxLen(/* int len */ atoi(line + 11))

       : line[0] == '/' ? printf("Unreconized command: %s\n", line) : (void)NULL;

    free(line);
  }

  return EXIT_SUCCESS;
}
