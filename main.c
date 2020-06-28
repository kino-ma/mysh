#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "token.h"

#define MAXARG 20


token *get_cmd();
void exec_cmd(token *);
void exec_cmd_pipe(token *prev, token *next);



int main() {
    pid_t child_pid = 0;
    int status;
    token *cmd_head;

    while (1) {
        fprintf(stderr, "$ ");

        cmd_head = get_cmd();

        if (cmd_head == NULL || cmd_head->next == NULL) {
            continue;
        }

        child_pid = fork();

        if (child_pid < 0) {
            fprintf(stderr, "failed to fork\n");
            return 1;
        }


        if (child_pid > 0) {
            // parent
            waitpid(child_pid, &status, 0);

            // if child exited with signal
            if (WIFSIGNALED(status)) {
                printf("command exited with status: %d\n", WTERMSIG(status));
            }

            terminate(cmd_head);
        } else {
            int pgid = setpgid(0, 0);
            // child
            exec_cmd(cmd_head);
        }
    }
}




token *get_cmd() {
    char *input = NULL;
    size_t n;

    // if failed to get input
    if (getline(&input, &n, stdin) == -1) {
        fprintf(stderr, "failed to get input\n");
        return NULL;
    }

    token *head = tokenize(input);

    return head;
}



void exec_cmd(token *head) {
    char *command, *args[MAXARG];
    int argc = 0;

    token *start = head;
    while (start->type == TOKEN_HEAD) {
        start = start->next;
    }

    for (token *cur = start; cur != NULL; cur = cur->next) {
        /* normal token */
        if (cur->type == WORD) {
            args[argc] = cur->word;
            argc++;
        }

        /* redirect */
        if (cur->type == REDIRECT_OW || cur->type == REDIRECT_ADD) {
            int opt;
            opt = O_WRONLY | O_CREAT;

            if (cur->type == REDIRECT_OW) {
                opt |= O_TRUNC;
            } else if (cur->type == REDIRECT_ADD) {
                opt |= O_APPEND;
            }

            cur = cur->next;
            char *filename = cur->word;
            int fd = open(filename, opt, 0644);

            if (fd < 0) {
                fprintf(stderr, "failed to open file: %s\n", filename);
                fprintf(stderr, "%s\n", strerror(errno));
                exit(1);
            }

            dup2(fd, 1);
            close(fd);

            continue;
        }

        /* pipe */
        if (cur->type == PIPE) {
            token *next = cur->next;
            cur = cur->prev;
            erase(cur->next);
            cur->next = NULL;
            exec_cmd_pipe(head, next);
        }
    }

    args[argc] = NULL;

    execvp(args[0], args);

    // if returned
    fprintf(stderr, "exec %s failed\n", args[0]);
    perror("exec");
    exit(1);
}


void exec_cmd_pipe(token *prev, token *next) {
    // create pipe connected to self
    int fds[2];
    pipe(fds);

    pid_t pid = fork();

    /* failed */
    if (pid < 0) {
        fprintf(stderr, "failed to fork\n");
        exit(1);
    }

    /* parent */
    /* input from child */
    if (pid > 0) {
        // close pipe (write)
        close(fds[1]);

        // move pipe (read) to stdin and close
        dup2(fds[0], 0);
        close(fds[0]);

        // wait child
        waitpid(pid, NULL, 0);

        // exec
        exec_cmd(next);
    }

    /* child */
    /* output to parent */
    if (pid == 0) {
        // close pipe (read)
        close(fds[0]);

        // move pipe (write) to stdout and close
        dup2(fds[1], 1);
        close(fds[1]);

        // exec
        exec_cmd(prev);
    }
}
