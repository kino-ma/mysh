#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

token *new_token(char *str, int d, enum token_type type) {
    token *new = malloc(sizeof(token));
    new->type = type;

    if (type == TOKEN_HEAD) {
        return new;
    }

    new->word = calloc(d+1, sizeof(char));

    strncpy(new->word, str, d+1);
    new->word[d] = '\0';
    new->next = NULL;

    return new;
}

token *add_token(token *cur, token *next) {
    cur->next = next;
    next->prev = cur;

    return next;
}

token *tokenize(char *str) {
    if (strcmp("\n", str) == 0) {
        return NULL;
    }

    enum token_type type;
    token *head, *cur;
    cur = head = new_token(NULL, 0, TOKEN_HEAD);

    char *s = index(str, '\n');
    *s = '\0';

    for (int i = 0, j = i; i < strlen(str); j++) {
        type = WORD;

        // skip spaces
        if (str[i] == ' ' || str[i] == '\0') {
            i++;
            continue;
        }

        j = i + 1;
        switch (str[i]) {
            case '>':
                type = REDIRECT_OW;
                if (str[i+1] == '>') {
                    type = REDIRECT_ADD;
                    j += 1;
                }
                break;
            case '<':
                type = READ_FILE;
                if (str[i+1] == '<') {
                    type = HEREDOC;
                    j += 1;
                    if (str[i+2] == '<') {
                        type = HERESTR;
                        j += 1;
                    }
                }
                break;
            case '|':
                type = PIPE;
                break;
            default:
                type = WORD;
                while (index("<>| \0", str[j]) == NULL) {
                    j += 1;
                }
                break;
        }
        cur = add_token(cur, new_token(str + i, j - i, type));
        i = ++j;
    }

    return head;
}



void erase(token *t) {
    if (t->type != TOKEN_HEAD) {
        free(t->word);
    }
    free(t);
}

void terminate(token *head) {
    token *cur, *next;
    for (cur = head; cur != NULL; cur = next) {
        next = cur->next;
        erase(cur);
    }
}
