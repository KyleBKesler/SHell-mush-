#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parseline.h"

/* malloc function to ensure memory is properly allocated */
void *safe_malloc(int size) {
    void *new;
    if (!(new = malloc(size))) {
        perror("Malloc");
        exit(EXIT_FAILURE);
    }
    return new;
}

/* free the parsed structure as well as the variables inside */
/* the structure that were malloced */
void free_parsed(struct Parse *parsed, int num_stages) {
    int i; int j;
    for (i = 0; i <= num_stages; i++) {
        free(parsed[i].stage_name);
        free(parsed[i].input);
        free(parsed[i].output);
        for (j = 0; j < parsed[i].num_arg; j++) {
            if (parsed[i].args[j] != NULL) {
                free(parsed[i].args[j]);
            }
        }
    } 
    free(parsed);
}

/*print out the parsed command following the guidelines and */
/* format for propper output */
/* *** NOT USED IN MUSH *** */
void print_parsed(struct Parse *parsed, int num_stages) {
    int i; int j;
    for (i = 0; i < num_stages + 1; i++) {
        fprintf(stdout, "\n--------\n");
        fprintf(stdout, "Stage %d: \"%s\"", parsed[i].stage, parsed[i].stage_name);
        fprintf(stdout, "\n--------\n");
        fprintf(stdout, "     input: %s\n", parsed[i].input);
        fprintf(stdout, "    output: %s\n", parsed[i].output);
        fprintf(stdout, "      argc: %d\n", parsed[i].num_arg);
        fprintf(stdout, "      argv: \"%s\"", parsed[i].args[0]);
        for (j = 1; j < parsed[i].num_arg; j++) {
            fprintf(stdout, ",\"%s\"", parsed[i].args[j]);    
        }
        fprintf(stdout, "\n");
    }
}

int parseline(char *command, Parse *parsed) {
    char input[LEN_IN] = "original stdin";
    char output[LEN_IN] = "original stdout";
    char stage_name[CMD_LENGTH] = {0};
    int stage = 0; 
    int num_cmd_args = 0;
    int argv_cmd = 0; 
    int o_redirect = 0;
    int i_redirect = 0; 
    int pipe = 0;

    /* malloc space for struct and variables that will hold strings */
    parsed[0].stage = 0;
    parsed[0].num_arg = 0;
    parsed[0].stage_name = (char *)safe_malloc(sizeof(char) * CMD_LENGTH + 1);
    parsed[0].input = (char *)safe_malloc(sizeof(char) * LEN_IN + 1);
    parsed[0].output = (char *)safe_malloc(sizeof(char) * LEN_IN + 1);
    strcpy(parsed[0].input, input);
    strcpy(parsed[0].output, output);

    /* clear white space from command and iterate through non white */
    /* space string. If no input return to mush */
    char *arg = strtok(command, " ");
    if (arg == NULL) {
        return 0;
    }
    while (arg != NULL) {
        if ((*arg == '|') && (num_cmd_args == 0)) {
            fprintf(stderr, "Invalid Null Command\n");
            return 0;
        }
        /* pipe. check for proper conditions to pipe to next stage */
        /* initialze and malloc space for next stage variables */
        /* and reset variables */
        else if (*arg == '|') {
            if (stage >= 9) {
                fprintf(stderr, "Pipeline too deep.\n");
                return 0;
            }
            if (o_redirect) {
                fprintf(stderr, "%s: Ambiguous Output\n", parsed[stage].args[0]);
                return 0;
            }
            sprintf(output, "pipe to stage %d", stage + 1);
            strcpy(parsed[stage].output, output);
            sprintf(input, "pipe from stage %d", stage);

            stage++;
            parsed[stage].input = (char *)safe_malloc(sizeof(char) * LEN_IN + 1);
            parsed[stage].output = (char *)safe_malloc(sizeof(char) * LEN_IN + 1);
            parsed[stage].stage_name = (char *)safe_malloc(sizeof(char) * CMD_LENGTH + 1);
            strcpy(parsed[stage].input, input);
            strcpy(parsed[stage].output, "original stdout");
            stage_name[0] = '\0';

            parsed[stage].stage = stage;
            parsed[stage].num_arg = 0;
            num_cmd_args = 0;
            i_redirect = 0;
            o_redirect = 0;
            pipe = 1;
        }
        /* redirect out. set variables so that next arg goes into output */
        /* add to the stage_name */
        else if (*arg == '>') {
            if (o_redirect) {
                fprintf(stderr, "%s: Bad Ouput Redirection\n", parsed[stage].args[0]);
                return 0;
            }
            o_redirect = 1;
            argv_cmd = 2;
            strcat(stage_name, " ");
            strcat(stage_name, arg);
            strcat(stage_name, " ");
        }
        /* redirect in. set variables so that next arg goes into input */
        /* add to the stage_name */
        else if (*arg == '<') {
            if (i_redirect) {
                fprintf(stderr, "%s: Bad Input Redirection\n", parsed[stage].args[0]);
                return 0;
            }
            if (pipe) {
                fprintf(stderr, "%s: Ambiguous Input\n", parsed[stage].args[0]);
                return 0;
            }
            i_redirect = 1;
            argv_cmd = 1;
            strcat(stage_name, " ");
            strcat(stage_name, arg);
            strcat(stage_name, " ");
        }
        /* adds arg to the string_name and list of args in this stage */
        /* if the previous arg was redirect then string_name is */
        /* updated and input/output is updated */
        else {
            if (num_cmd_args - 1 >= CMD_ARG) {
                fprintf(stderr, "%s: Too many arguments.\n", parsed[stage].args[0]);
                return 0;
            }
            if (argv_cmd == 0) {
                if (num_cmd_args > 0) {
                    strcat(stage_name, " ");
                }
                strcat(stage_name, arg);
                strcpy(parsed[stage].stage_name, stage_name);
                parsed[stage].args[num_cmd_args] = (char *)safe_malloc(sizeof(char) * LEN_IN + 1);
                strcpy(parsed[stage].args[num_cmd_args], arg);
                parsed[stage].num_arg++;
                num_cmd_args++;
            }
            else if (argv_cmd == 1) {
                strcpy(parsed[stage].input, arg);
                strcat(stage_name, arg);
                strcpy(parsed[stage].stage_name, stage_name);
                argv_cmd = 0;
            }
            else if (argv_cmd == 2) {
                strcpy(parsed[stage].output, arg);
                strcat(stage_name, arg);
                strcpy(parsed[stage].stage_name, stage_name);
                argv_cmd = 0;
            }
        }
        /* move to next arg */
        arg = strtok(arg + strlen(arg) + 1, " ");
    } 
    return stage + 1;
}
