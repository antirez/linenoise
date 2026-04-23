#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define KEY_CTRL_A  "\x01"
#define KEY_CTRL_E  "\x05"
#define KEY_CTRL_K  "\x0b"
#define KEY_CTRL_Y  "\x19"
#define KEY_LEFT    "\x1b[D"
#define KEY_ENTER   "\r"

int pipe_to_child[2];
int pipe_from_child[2];

void send_keys(const char *keys) {
    write(pipe_to_child[1], keys, strlen(keys));
    usleep(100000);
}

void read_output(void) {
    char buf[4096];
    fd_set rfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&rfds);
    FD_SET(pipe_from_child[0], &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    retval = select(pipe_from_child[0] + 1, &rfds, NULL, NULL, &tv);
    if (retval > 0) {
        int n = read(pipe_from_child[0], buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("OUTPUT (%d bytes): ", n);
            for (int i = 0; i < n; i++) {
                if (buf[i] >= 32 && buf[i] < 127) {
                    printf("%c", buf[i]);
                } else if (buf[i] == '\r') {
                    printf("<CR>");
                } else if (buf[i] == '\n') {
                    printf("<LF>");
                } else if (buf[i] == 0x1b) {
                    printf("<ESC>");
                } else {
                    printf("<0x%02x>", (unsigned char)buf[i]);
                }
            }
            printf("\n");
        }
    }
}

int main() {
    if (pipe(pipe_to_child) == -1 || pipe(pipe_from_child) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        close(pipe_to_child[1]);
        close(pipe_from_child[0]);
        dup2(pipe_to_child[0], STDIN_FILENO);
        dup2(pipe_from_child[1], STDOUT_FILENO);
        dup2(pipe_from_child[1], STDERR_FILENO);
        close(pipe_to_child[0]);
        close(pipe_from_child[1]);
        execl("./linenoise-example", "./linenoise-example", (char *)NULL);
        perror("execl");
        exit(1);
    }

    close(pipe_to_child[0]);
    close(pipe_from_child[1]);

    printf("=== Debug Test: Ctrl+K Behavior ===\n\n");

    printf("Step 1: Read initial prompt\n");
    read_output();

    printf("\nStep 2: Type 'hello world'\n");
    send_keys("hello world");
    read_output();

    printf("\nStep 3: Ctrl+A (home) then Ctrl+E (end)\n");
    send_keys(KEY_CTRL_A KEY_CTRL_E);
    read_output();

    printf("\nStep 4: Press LEFT 5 times\n");
    for (int i = 0; i < 5; i++) {
        send_keys(KEY_LEFT);
        read_output();
    }

    printf("\nStep 5: Press Ctrl+K (kill to end)\n");
    send_keys(KEY_CTRL_K);
    read_output();

    printf("\nStep 6: Press Enter to submit\n");
    send_keys(KEY_ENTER);
    read_output();
    read_output();

    printf("\n=== End of Debug Test ===\n");

    close(pipe_to_child[1]);
    close(pipe_from_child[0]);
    waitpid(pid, NULL, 0);

    return 0;
}
