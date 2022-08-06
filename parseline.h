#ifndef PARSELINE_H
#define PARSELINE_H

#define CMD_LENGTH 512
#define CMD_PIPE 10
#define CMD_ARG 10
#define LEN_IN 20
#define PROMPT 4

/* struct to hold parsed command values */                                    
typedef struct Parse {
    int stage;
    char *stage_name;
    char *input;
    char *output;
    int num_arg;
    char *args[CMD_ARG];
} Parse;

void *safe_malloc(int size);
void free_parsed(struct Parse *parsed, int num_stages);
void print_parsed(struct Parse *parsed, int num_stages);
int parseline(char *command, Parse *parsed);

#endif
