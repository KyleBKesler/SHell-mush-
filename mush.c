#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include "parseline.h"

#define CMP_SIZE 4

/* variable to process correct interrupt handling */
static int interrupt = 0;

/* interrupt handler kills children and sets up for */
/* next stdin input */
void handler() {
    interrupt = 1;
    killpg(getpid(), SIGINT);
    signal(SIGINT, handler);
    write(STDOUT_FILENO, "\n8-P ", PROMPT + 1);
}

/* handles the execution of the commands */
int execute(Parse *parsed, int num_stgs) {
    int i; int j;
    int out_fd; int in_fd;
    int pipefd[CMD_PIPE * 2];
    pid_t child;
    int status;
    int num_pipe = 0;
    int dup = 0;

    /* create pipes depending on the number of stages */
	for (i = 0; i < num_stgs - 1; i++) {
		if (-1 == pipe(&pipefd[num_pipe])) {
            fprintf(stderr, "Error creating pipes.\n");
            return 0;
        }
        num_pipe += 2;
	}

    /* iterate through each stage and execute the commands */
    for (j = 0; j < num_stgs; j++) {
        /* check if command is 'cd', if so change directories */
        /* and return to main to reprompt user/read next line */
        if (!strcmp(parsed[j].args[0], "cd")) {
            if (j != 0){
                fprintf(stderr, "cd: cannot change directories in a pipeline\n");
                break;
            }
			if (-1 == chdir(parsed[j].args[1])) {
                perror(parsed[j].args[1]);
            }
            return 0;
		}
        else if (!(child = fork())) {
            /* child process */
            /* check if piping to input from previous stage */
            /* if not check for input redirects */
            /* make sure dup works */
            if (!strncmp(parsed[j].input, "pipe", CMP_SIZE)) {
                if (-1 == (dup = dup2(pipefd[(j - 1) * 2], STDIN_FILENO))) {
                    perror("dup input pipe");
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(parsed[j].input, "original stdin")) {
                if (-1 == (in_fd = open(parsed[j].input, O_RDONLY))) {
                    perror(parsed[j].input);
                    exit(EXIT_FAILURE);
                }
                if (-1 == (dup = dup2(in_fd, STDIN_FILENO))) {
                    perror("dup input file");
                    exit(EXIT_FAILURE);
                }
                if (-1 == close(in_fd)) {
                    perror("input file");
                    exit(EXIT_FAILURE);
                }
            }
            /* check if piping output to next stage */
            /* if not check for output redirects */
            /* make sure dup works */
            if (!strncmp(parsed[j].output, "pipe", CMP_SIZE)) {
                if (-1 == (dup = dup2(pipefd[(j * 2) + 1], STDOUT_FILENO))) {
                    perror("dup output pipe");
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(parsed[j].output, "original stdout")) {
                if (-1 == (out_fd = open(parsed[j].output,  O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR))) {
                    perror(parsed[j].output);
                    exit(EXIT_FAILURE);
                }
                if (-1 == (dup = dup2(out_fd, STDOUT_FILENO))) {
                    perror("dup output file");
                    exit(EXIT_FAILURE);
                }
                if (-1 == close(out_fd)) {
                    perror("output file");
                    exit(EXIT_FAILURE);
                }
            }
            /* close opened pipes */
            for (i = 0; i < num_pipe; i++) {
                if (-1 == close(pipefd[i])) {
                    perror("close child");
                    exit(EXIT_FAILURE);
                }
            }
            /* execute the command with corresponding args */
            execvp(parsed[j].args[0], parsed[j].args);
            perror(parsed[j].args[0]);
            exit(EXIT_SUCCESS);  
        }
        else {
            /* if unable to fork report the error */
            if (-1 == child) {
                perror("fork");
                exit(EXIT_FAILURE);
            }
        }      
    }
    /* parent process */
    /* close pipes that were opened */
    for (i = 0; i < num_pipe; i++) {
        if (-1 == close(pipefd[i])) {
            perror("close parent");
            exit(EXIT_FAILURE);
        }
    }
    /* wait for child processes to finish for each stage */
    /* check if interrupted by SIGINT */
    for (i = 0; i < num_stgs; i++) {
        if (-1 == wait(&status)) {
            perror("wait");
            exit(EXIT_FAILURE);
        }  
        /*
        if((errno != 0) && (errno != EINTR)){
            perror("error with waitpid");
            exit(EXIT_FAILURE);
        }
        */
        if (!WIFEXITED(status) || WEXITSTATUS(status)) {}
    }
    return 0;
}

int main(int argc, char *argv[]) {
    FILE *fp;
    int num_read;
    char command[CMD_LENGTH + 1];
    char temp[CMD_LENGTH];
    int stage;

    /* setup signal handling */
    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags   = 0;
    sigemptyset(&sa.sa_mask);
    if (-1 == sigaction(SIGALRM, &sa, NULL)) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    signal(SIGINT, handler);
    
    /* too many arguments */
    if (argc > 2) {
        fprintf(stderr, "usage: mush [ scriptfile ]\n");
        exit(EXIT_FAILURE);
    }
    /* if reading from a file read one command line at a time */
    /* and execute the commands */
    else if (argc == 2) {
        fp = fopen(argv[1], "r");
        if (!fp) {
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }
        fgets(command, CMD_LENGTH + 1, fp);
        while (!feof(fp)) {
            if (strlen(command) > CMD_LENGTH) {
                fprintf(stderr, "Command too long\n");
                break;
            }
            command[strlen(command) - 1] = '\0';
            Parse *parsed = (Parse *)safe_malloc(sizeof(Parse) * CMD_PIPE);
            stage = parseline(command, parsed);
            execute(parsed, stage);
            free_parsed(parsed, stage);
            fgets(command, CMD_LENGTH + 1, fp);
        }
        fclose(fp);
        if (feof(fp)) {
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }
    }
    /* reading from stdin */
    /* handles signal interrupts to wait for children to die if ^C */
    /* and reset/reprompt user. if ^D terminate */
    else {
        while (1) {
            if (!interrupt) {
                if (-1 == write(STDOUT_FILENO, "8-P ", PROMPT)) {
                    fprintf(stderr, "Error writing prompt");
                    break;
                }
                interrupt = 0;
            }    
            if (1 > (num_read = read(STDIN_FILENO, command, CMD_LENGTH + 1))) {
                if (!num_read) {
                    break;
                }
                fprintf(stderr, "Error reading input.\n");
            }
            else if (num_read > CMD_LENGTH) {
                fprintf(stderr, "Command too long.\n");
            }
            else {
                command[num_read - 1] = '\0';
                char *cmd = strtok(command, "\n");
                while (cmd != NULL) {
                    strncpy(temp, cmd, sizeof(temp));
                    Parse *parsed = (Parse *)safe_malloc(sizeof(Parse) * CMD_PIPE);
                    stage = parseline(temp, parsed);
                    //print_parsed(parsed, stage - 1);
                    if (stage != 0) {
                        execute(parsed, stage);
                    }
                    free_parsed(parsed, stage);
                    cmd = strtok(NULL, "\n");
                    //fprintf(stderr, "here: %s\n", cmd);
                }
                command[0] = '\0';
            }
        }
    }
    return 0;
}
