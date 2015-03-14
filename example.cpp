#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"

#define NUM_OF(x) ((sizeof(x))/(sizeof(x[0])))
#define UNUSED __attribute__((unused))

typedef void (*command_action_t)(const char *line);
typedef struct command_s {
    const char *c_token;
    const char *c_help;
    command_action_t c_action;
} command_t;

/*
 * Syntax for declaring a function pointer
 * between the '<>' enter the return type and function
 * arguments.
 */
typedef std::function<void (command_t *, const char *buf)> complete_cb;

static void help_cmd_action(const char *);
static void quit_cmd_action(const char *);
static void generic_cmd_action(const char *);

command_t cmds[] = {
    { "hello", "help for hello", help_cmd_action },
    { "helo",  "help for helo",  help_cmd_action },
    { "joe",   "help for joe",   generic_cmd_action },
    { "james", "help for james", generic_cmd_action },
    { "quit",  "quit from test", quit_cmd_action },

    { "blah0", "help for blah0", generic_cmd_action },
    { "blah1", "help for blah1", generic_cmd_action },
    { "blah2", "help for blah2", generic_cmd_action },
    { "blah3", "help for blah3", generic_cmd_action },
    { "blah4", "help for blah4", generic_cmd_action },
    { "blah5", "help for blah5", generic_cmd_action },
    { "blah6", "help for blah6", generic_cmd_action },
    { "blah7", "help for blah7", generic_cmd_action },
    { "blah8", "help for blah8", generic_cmd_action },
    { "blah9", "help for blah9", generic_cmd_action },
    { "blah10", "help for blah10", generic_cmd_action },
    { "blah11", "help for blah11", generic_cmd_action },
    { "blah12", "help for blah12", generic_cmd_action },
    { "blah13", "help for blah13", generic_cmd_action },
    { "blah14", "help for blah14", generic_cmd_action },
    { "blah15", "help for blah15", generic_cmd_action },
    { "blah16", "help for blah16", generic_cmd_action },
    { "blah17", "help for blah17", generic_cmd_action },
    { "blah18", "help for blah18", generic_cmd_action },
    { "blah19", "help for blah19", generic_cmd_action },
    { "blah20", "help for blah20", generic_cmd_action },
    { "blah21", "help for blah21", generic_cmd_action },
    { "blah22", "help for blah22", generic_cmd_action },
    { "blah23", "help for blah23", generic_cmd_action },
    { "blah24", "help for blah24", generic_cmd_action },
    { "blah25", "help for blah25", generic_cmd_action },
    { "blah26", "help for blah26", generic_cmd_action },
    { "blah27", "help for blah27", generic_cmd_action },
    { "blah28", "help for blah28", generic_cmd_action },
    { "blah29", "help for blah29", generic_cmd_action },
    { "blah30", "help for blah30", generic_cmd_action },
    { "blah31", "help for blah31", generic_cmd_action },
    { "blah32", "help for blah32", generic_cmd_action },
    { "blah33", "help for blah33", generic_cmd_action },
    { "blah34", "help for blah34", generic_cmd_action },
    { "blah35", "help for blah35", generic_cmd_action },
    { "blah36", "help for blah36", generic_cmd_action },
    { "blah37", "help for blah37", generic_cmd_action },
    { "blah38", "help for blah38", generic_cmd_action },
    { "blah39", "help for blah39", generic_cmd_action },
};

void
help_cmd_action (const char *line)
{
    printf("help for '%s'\n", line);
}

void
generic_cmd_action (const char *line)
{
    printf("generic for '%s'\n", line);
}

void
quit_cmd_action (const char *line UNUSED)
{
    exit(0);
}
static int
match_prefix (const char *whole, const char *partial) 
{
    return partial == NULL || strlen(partial) == 0 ||
	strncmp(whole, partial, strlen(partial)) == 0;
}

/*
 * supporting routine to find a matching command in the 'cmds_table'
 */
void
completion (const char *buf, complete_cb cb)
{
    unsigned int i;

    for (i = 0; i < NUM_OF(cmds); i++) {
	if (match_prefix(cmds[i].c_token, buf))
	    cb(&cmds[i], buf);
    }
}

void
completion_ln (const char *buf, linenoiseCompletions *lc)
{
    completion(buf, [lc] (command_t *cmd, const char *buf UNUSED) -> void {
	    linenoiseAddCompletion(lc, cmd->c_token, cmd->c_help);
	}
    );
}

void
call_command (const char *buf)
{
    int n_commands = 0;
    command_t *found_cmd;
    
    completion(buf, [&] (command_t *cmd, const char *buf UNUSED) {
	    found_cmd = cmd;
	    n_commands++;
	});

    switch (n_commands) {
    case 0:
	printf("no commands matched\n");
	break;
    case 1:
	linenoiseHistoryAdd(buf);
	linenoiseHistorySave("history.txt");

	found_cmd->c_action(buf);
	break;
    default:
	printf("More than one command matched\n");
	break;
    }
}

int
main (int argc, char **argv) 
{
    char *line;
    char *prgname = argv[0];

    /* Parse options, with --multiline we enable multi line editing. */
    while (argc > 1) {
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

    linenoiseSetCompletionCallback(completion_ln);

    linenoiseHistoryLoad("history.txt");

    /*
     * Now this is the main loop of the typical linenoise-based application.
     * The call to linenoise() will block as long as the user types something
     * and presses enter.
     *
     * The typed string is returned as a malloc() allocated string by
     * linenoise, so the user needs to free() it. 
     */
    while ((line = linenoise("computer> ")) != NULL) {
	call_command(line);
        free(line);
    }
    return 0;
}
