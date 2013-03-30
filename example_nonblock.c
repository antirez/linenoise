#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/time.h>

#include "linenoise.h"

/* An alarm which is set to expire at some time in the future */
typedef struct {
    int id;
    struct timeval next;
} alarm_t;

/* Set alarm to be a given number of milliseconds ahead of the start time */
static void alarm_advance_by_ms(alarm_t *alarm, struct timeval *start, int ms) {
#   define US_PER_S     (1000 * 1000)
#   define US_PER_MS    (1000)

    suseconds_t new_usec = start->tv_usec + (US_PER_MS * ms);
    alarm->next.tv_usec = new_usec % US_PER_S;
    alarm->next.tv_sec = start->tv_sec + (new_usec / US_PER_S);
}

/* Get the number of milliseconds unitl this alarm expires */
static int alarm_ms_until(struct timeval *now, alarm_t *alarm) {
    return ((alarm->next.tv_sec - now->tv_sec) * 1000) + ((alarm->next.tv_usec - now->tv_usec) / 1000);
}

/* Find the next alarm that will expire */
static void alarm_find_next(alarm_t *alarms[], int num_alarms, alarm_t **next_alarm) {
    int i;
    *next_alarm = NULL;

    alarm_t *best = NULL;
    for (i = 0; i < num_alarms; i++) {
        alarm_t *alarm = alarms[i];
        if (best == NULL ||
                (alarm->next.tv_sec < best->next.tv_sec || 
                 (alarm->next.tv_sec == best->next.tv_sec && 
                    alarm->next.tv_usec < best->next.tv_usec))) {
           best = alarm;
        }
    }

    *next_alarm = best;
}

void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == 'h') {
        linenoiseAddCompletion(lc,"hello");
        linenoiseAddCompletion(lc,"hello there");
    }
}

int main(int argc, char **argv) {
    char *line;
    char *prgname = argv[0];
    int rc;

    /* Parse options, with --multiline we enable multi line editing. */
    while(argc > 1) {
        argc--;
        argv++;
        if (!strcmp(*argv,"--multiline")) {
            linenoiseSetMultiLine(1);
            printf("Multi-line mode enabled.\n");
        } else {
            fprintf(stderr, "Usage: %s [--multiline]\n", prgname);
            exit(1);
        }
    }

    /* Set the completion callback. This will be called every time the
     * user uses the <tab> key. */
    linenoiseSetCompletionCallback(completion);

    /* Load history from file. The history file is just a plain text file
     * where entries are separated by newlines. */
    linenoiseHistoryLoad("history.txt"); /* Load the history at startup */

    /* Now this is the main loop of the typical linenoise-based application.
     * The call to linenoise() will block as long as the user types something
     * and presses enter.
     *
     * The typed string is returned as a malloc() allocated string by
     * linenoise, so the user needs to free() it. */

    linenoiseContext *context = NULL;

    char prompt[100];
    int inactive_periods = 0;
    snprintf(prompt, sizeof (prompt), "nonblock-%d> ", inactive_periods);

    const int stdin_fd = STDIN_FILENO;
    rc = linenoiseSetupContext(&context, stdin_fd, prompt);
    if (rc) {
        printf("unable to setup context!");
        exit(1);
    }

    /* Setup the fd's we'll be polling on (just stdin for now) */
    struct pollfd poll_fds[1] = {};
    const int nfds = 1;

    poll_fds[0].fd = stdin_fd;
    poll_fds[0].events = POLLIN;

    /* Setup some activity alarms */
    alarm_t inactive, periodic;
    alarm_t *alarms[] = { &inactive, &periodic };
    const int num_alarms = sizeof (alarms) / sizeof (alarms[0]);

    memset((void *) &inactive, 0, sizeof (inactive));
    memset((void *) &periodic, 0, sizeof (periodic));

    inactive.id = 0;
    periodic.id = 1;

    const int inactive_ms_timeout = 2000;
    const int periodic_ms_timeout = 5000;

    struct timeval now;
    gettimeofday(&now, NULL);

    alarm_advance_by_ms(&inactive, &now, inactive_ms_timeout);
    alarm_advance_by_ms(&periodic, &now, periodic_ms_timeout);

    linenoiseStartInput(context);

    while (1) {
        alarm_t *alarm;
        alarm_find_next(alarms, num_alarms, &alarm);

        /* Set the poll(2) timeout to coincide with the expirey of the next alarm */
        int ms_timeout = alarm_ms_until(&now, alarm);
        int num_events = poll((struct pollfd *) &poll_fds, nfds, ms_timeout);

        gettimeofday(&now, NULL);

        /* No events meant the timeout expired */
        if (num_events == 0) {
            /* Clear the line of any editing before showing more */
            linenoiseClearLine(context);

            if (alarm->id == inactive.id) {
                printf("*** Inactivity timeout alarm (after %d ms of no interaction)\r\n", inactive_ms_timeout);
                alarm_advance_by_ms(&inactive, &now, inactive_ms_timeout);

                inactive_periods++;
                snprintf(prompt, sizeof (prompt), "nonblock-%d> ", inactive_periods);
                linenoiseSetPrompt(context, prompt);
            }
            else if (alarm->id == periodic.id) {
                printf("*** Periodic alarm (every %d ms)\n\r", periodic_ms_timeout);
                alarm_advance_by_ms(&periodic, &now, periodic_ms_timeout);
            }

            /* Restore the editing prompt */
            linenoiseRefreshLine(context);
        }
        else if (poll_fds[0].revents & POLLIN) {
            /* Any stdin activity puts off the inactivity alarm expirey time */
            alarm_advance_by_ms(&inactive, &now, inactive_ms_timeout);

            char c;
            int nread = read(stdin_fd,&c,1);
            if (nread <= 0) break;

            rc = linenoiseHandleInput(context, c, &line);
            if (rc) break;

            if (line != NULL) {
                /* Do something with the string. */
                if (line[0] != '\0' && line[0] != '/') {
                    printf("echo: '%s'\n", line);
                    linenoiseHistoryAdd(line); /* Add to the history. */
                    linenoiseHistorySave("history.txt"); /* Save the history on disk. */
                } else if (!strncmp(line,"/historylen",11)) {
                    /* The "/historylen" command will change the history len. */
                    int len = atoi(line+11);
                    linenoiseHistorySetMaxLen(len);
                } else if (line[0] == '/') {
                    printf("Unreconized command: %s\n", line);
                }

                free(line);

                /* Re-enable editing for next time */
                linenoiseStartInput(context);
            }
        }
    }

    return 0;
}
