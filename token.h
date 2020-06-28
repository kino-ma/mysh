enum token_type {
    TOKEN_HEAD,
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


token *new_token(char *, int, enum token_type);
token *add_token(token *, token *);
token *tokenize(char *);
void erase(token *);
void terminate(token *);
