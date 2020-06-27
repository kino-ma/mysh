#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#define MAXARG 20

enum token_type {
    WORD,
    READ_FILE,
    HEREDOC,
    HERESTR,
    REDIRECT_OW,
    REDIRECT_ADD,
    PIPE,
};

typedef struct _token {
    struct _token *next;
    struct _token *prev;
    char *word;
    enum token_type type;
} token;


token *add_token(token *, char *, int, enum token_type);
token *tokenize(char *);
void exec_cmd(token *);
void exec_cmd_pipe(token *prev, token *next);
void terminate(token *heod);



token *add_token(token *cur, char *str, int d, enum token_type type) {
    cur->word = calloc(d, sizeof(char));

    strncpy(cur->word, str, d+1);
    cur->word[d] = '\0';

    cur->next = malloc(sizeof(token));
    cur->next->prev = cur;
    cur->type = type;

    return cur->next;
}

token *tokenize(char *str) {
    if (strcmp("\n", str) == 0) {
        return NULL;
    }

    token *head, *cur;
    cur = head = malloc(sizeof(token));

    char *s = index(str, '\n');
    *s = '\0';

    for (int i, j = i = 0; i < strlen(str); j++) {
        enum token_type type = WORD;

        // skip spaces
        while (str[i] == ' ' || str[i] == '\0') {
            i++, j++;
        }

        switch (str[i]) {
            case '>':
                type = REDIRECT_OW;
                if (str[i+1] == '>') {
                    type = REDIRECT_ADD;
                    j = i + 1;
                }
                break;
            case '<':
                type = READ_FILE;
                if (str[i+1] == '<') {
                    type = HEREDOC;
                    j = i + 1;
                    if (str[i+2] == '<') {
                        type = HERESTR;
                        j = i + 2;
                    }
                }
                break;
            case '|':
                type = PIPE;
                j = i + 1;
                break;
            default:
                type = WORD;
                while (index("<>| \0", str[j]) == NULL) {
                    j++;
                }
                break;
        }

        cur = add_token(cur, str + i, j - i, type);

        i = ++j;
    }

    return head;
}


token *get_cmd() {
    char *input = NULL;
    size_t n;

    // if failed to get input
    if (getline(&input, &n, stdin) == -1) {
        fprintf(stderr, "failed to get input\n");
        exit(1);
    }

    token *head = tokenize(input);

    return head;
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

        // move pipe (read) to stdin and close original one
        dup2(fds[0], 0);
        close(fds[0]);

        // wait child
        waitpid(pid, NULL, 0);

        // exec
        exec_cmd(next);
    }

    /* child */
    /* output to  parent */
    if (pid == 0) {
        // close pipe (read)
        close(fds[0]);

        // move pipe (write) to stdout and close original one
        dup2(fds[1], 1);
        close(fds[1]);

        // exec
        exec_cmd(prev);
    }
}



void exec_cmd(token *head) {
    char *command, *args[MAXARG];
    int argc = 0;
    for (token *cur = head; cur->next != NULL; cur = cur->next) {
        // normal token
        if (cur->type == WORD) {
            args[argc] = cur->word;
            argc++;
        }

        // redirect
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
            int fd = open(filename, opt);

            if (fd < 0) {
                fprintf(stderr, "failed to open file: %s\n", filename);
                fprintf(stderr, "%s\n", strerror(errno));
                exit(1);
            }

            close(1);
            dup2(fd, 1);
            close(fd);

            continue;
        }

        // pipe
        if (cur->type == PIPE) {
            token *next = cur->next;
            cur->next = NULL;
            exec_cmd_pipe(head, next);
        }
    }

    args[argc] = NULL;

    execvp(args[0], args);

    // if returned
    fprintf(stderr, "exec %s failed\n", head->word);
    perror("exec");
}

void terminate(token *head) {
    token *cur, *next;
    for (cur = head; cur->next != NULL; cur = next) {
        next = cur->next;
        free(cur->word);
        free(cur);
    }
}



int main() {
    pid_t child_pid = 0;
    int status;
    token *cmd_head;

    while (1) {
        fprintf(stderr, "$ ");

        cmd_head = get_cmd();

        if (cmd_head == NULL) {
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
                printf("status: %d\n", WTERMSIG(status));
            }

            terminate(cmd_head);
        } else {
            int pgid = setpgid(0, 0);
            // child
            exec_cmd(cmd_head);

        }

    }
}
