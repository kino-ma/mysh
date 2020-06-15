#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define MAXARG 20

char *get_cmd(int *argc_out, char *args[MAXARG]) {
    char *input = NULL;
    size_t n;

    // if failed to get input
    if (getline(&input, &n, stdin) == -1) {
        fprintf(stderr, "failed to get input\n");
        exit(1);
    }

    int argc = 0;
    for (int i, j = i = 0; j < strlen(input); j++) {
        // token
        if (input[j] == ' ' || input[j] == '\n') {
            args[argc] = calloc(j - i + 1, sizeof(char));
            strncpy(args[argc], input+i, j - i);

            args[argc][j-i] = '\0';
            i = ++j, argc++;
        }
    }

    args[argc] = NULL;
    *argc_out = argc;

    return args[0];
}

int main() {
    pid_t child_pid = 0;
    char *command, *args[MAXARG];
    int argc, status;

    while (1) {
        printf("$ ");

        command = get_cmd(&argc, args);

        printf("(parent)args:\n");
        for (int i = 0; i < argc; i++) {
            printf("'%s'\n", args[i]);
        }

        child_pid = fork();

        if (child_pid < 0) {
            fprintf(stderr, "failed to fork\n");
            return 1;
        }


        if (child_pid != 0) {
            // parent
            waitpid(child_pid, &status, 0);

            // if child exited with signal
            if (WIFSIGNALED(status)) {
                printf("status: %d\n", WTERMSIG(status));
            }

        } else {
            // child
            execv(command, args);

            // if returned
            fprintf(stderr, "failed to exec\n");
            return 1;
        }

    }
}
