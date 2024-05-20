#include "parser.h"
#include "lexer.h"
#include "executor.h"

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#define MAX_BIG_PROCS 256

BackgroundProcess backgroundProcesses[MAX_BIG_PROCS];
//global variables for background processing and updating 
static pid_t backgroundPIDs[MAX_BIG_PROCS];    
static int backgroundJobNumbers[MAX_BIG_PROCS];
static char* backgroundCommands[MAX_BIG_PROCS];
static int bgProcessCt = 0;

//for part 9 exit function
#define MAX_HISTORY 3
char *commandHistory[MAX_HISTORY] = {NULL, NULL, NULL};
int commandCount = 0;

//main executing function, accepts cmd from parser as token->items[0]
void execute(tokenlist *tokens){
	int status;
	pid_t pid = fork();									//fork!
	if (pid == 0){ 									//if child process
		if(tokens->size > 1){	  //if there is more than one token, run execv with token list. 
			execv(tokens->items[0], tokens->items);     //^token list has its own null buffer
		} else if(tokens->size == 1){ //if there is exactly one token, run and add a null buffer
			char *args[2] = {tokens->items[0], NULL};	
			execv(args[0], args);
		} else{										//case of having no command
			fprintf(stderr, "No command provided\n");
			exit(EXIT_FAILURE);
		}
	} else if (pid > 0){								//waitpid if return to parent
		waitpid(pid, &status, 0);
	} else{											//error if no child
		perror("Fork failed");
		exit(EXIT_FAILURE);
	}
}

void handleExit(tokenlist *tokens){
    int j = commandCount + 1;
    //this loop waits for background processes to finish
    for (int i = 0; i < bgProcessCt; i++){
        if (backgroundPIDs[i] != -1){
            int status;
            waitpid(backgroundPIDs[i], &status, 0);
        }
    }
    //this displays the last three valid commands
    if (commandCount == 0){
        printf("No valid commands were entered.\n");
    } else{
        printf("Last %d valid commands:\n", commandCount);
        for (int i = MAX_HISTORY - 1; i >= 0; i--){
            if (commandHistory[i] != NULL){
                j--;
                printf("[%d]: %s\n", j, commandHistory[i]);
                free(commandHistory[i]);
                commandHistory[i] = NULL;
            }
        }
    }
    exit(0);
}

void updateCommandHistory(char* command){
    //frees the oldest command if necessary
    if (commandHistory[MAX_HISTORY - 1] != NULL){
        free(commandHistory[MAX_HISTORY - 1]);
    }
    for (int i = MAX_HISTORY - 1; i > 0; i--){
        commandHistory[i] = commandHistory[i - 1];
    }

    //add new command at the start of the history
    commandHistory[0] = my_strdup(command);
    commandCount++;
}

void handleCDPATH(tokenlist *tokens){					//handles cd token in part 9
	if(tokens->size == 1){
		int test = chdir(getenv("HOME"));
        if (test == -1){//meets req, if no argument, set directory to $HOME
			fprintf(stderr, "Directory switch failed\n");
		}
	} else if(tokens->size == 2){			//the cd command otherwise only has one argument.
		int test = chdir(tokens->items[1]); //the last one will be the path
		if (test == -1){
			//clarify whether or not we need separate error messages for not a directory and dne
			fprintf(stderr, "Not a directory or does not exist\n");
		}
	} else{												//cd only has one argument. if there are
		fprintf(stderr, "Too many arguments\n");        //more than 2 tokens, throw an error
	}
}

void handleJobs() {                                     //handles jobs token in part 9
    bool foundActiveJobs = false;

    for (int i = 0; i < bgProcessCt; i++) {
        // Assuming a PID of -1 indicates an inactive or completed job
        if (backgroundPIDs[i] != -1) {
            foundActiveJobs = true;
            printf("[%d]+ %d %s\n", backgroundJobNumbers[i], backgroundPIDs[i], 
                   backgroundCommands[i] ? backgroundCommands[i] : "Unknown Command");
        }
    }
    if (!foundActiveJobs) {
        printf("No active background processes.\n");
    }
}


void outputRedirect(tokenlist *tokens){					//handles output redirect
	char *outputFile;
	for(int i = 0; i < tokens->size; i++){				//this for loop searches for the >, and
		if(strcmp(tokens->items[i], ">") == 0){			//when it finds it, it puts the 
			outputFile = tokens->items[i + 1];          //token that is one after it as 
			tokens->items[i] = NULL;                    //output file
		}
	}
	pid_t pid = fork();
 
	if(pid == 0){
		close(STDOUT_FILENO);
		int fd = open(outputFile,O_RDWR | O_CREAT, S_IRUSR);

		execv(tokens->items[0],tokens->items);
	} else{
		waitpid(pid, 0, 0);
	}
}

void inputRedirect(tokenlist *tokens){
	char * inputFile = NULL; //placeholder for input file
	for(int i = 0; i < tokens->size; i++){              //this for loop searches for the >, and
		if(strcmp(tokens->items[i], "<") == 0){         //when it finds it, it puts the 
			inputFile = tokens->items[i + 1];           //token that is one after it
			tokens->items[i] = NULL;                    //as input file
		}
	}

	pid_t pid = fork(); //fork!

	if(pid == 0){
		close(STDIN_FILENO);
		int fd = open(inputFile, O_RDONLY);

		execv(tokens->items[0],tokens->items);
	} else{
		waitpid(pid, 0, 0);
	}
}

void ioRedirect(tokenlist *tokens){
	char *inputFile = NULL;  //placeholders for the input and
    char *outputFile = NULL; //output files
    int inputIndex = -1, outputIndex = -1;

    //finds positions of input and output redirection symbols and their respective file names
    for (int i = 0; i < tokens->size; i++){
        if (strcmp(tokens->items[i], "<") == 0 && tokens->items[i + 1] != NULL){
            inputFile = tokens->items[i + 1];
            inputIndex = i;
        } else if (strcmp(tokens->items[i], ">") == 0 && tokens->items[i + 1] != NULL){
            outputFile = tokens->items[i + 1];
            outputIndex = i;
        }
    }

    if (!inputFile || !outputFile){ //throws error if missing file
        fprintf(stderr, "Error: Missing input or output file for redirection.\n");
        return;
    }

    int ifd = open(inputFile, O_RDONLY); //opens input file
    if (ifd == -1){
        perror("Failed to open input file");
        return;
    }

    int ofd = open(outputFile,O_RDWR | O_CREAT, S_IRUSR); //opens output file
    if (ofd == -1){
        perror("Failed to open output file");
        close(ifd);
        return;
    }

    pid_t pid = fork(); //you guessed it, fork!

    if (pid == -1){ //throws error if no child created
        perror("Failed to fork");
        close(ifd);
        close(ofd);
        return;
    }

    if (pid == 0){
        if (dup2(ifd, STDIN_FILENO) == -1){ //redirects output
            perror("Failed to redirect stdin");
            exit(EXIT_FAILURE);
        }

        if (dup2(ofd, STDOUT_FILENO) == -1){ //redirects input
            perror("Failed to redirect stdout");
            exit(EXIT_FAILURE);
        }

        close(ifd); //closes input and output
        close(ofd);

        //construct a new argument list excluding redirection symbols and filenames
        char *newArgs[tokens->size + 1]; //allocate space for maximum possible arguments
        int j = 0;
        for (int i = 0; i < tokens->size; i++){
            if(i != inputIndex && i != inputIndex + 1 && i != outputIndex && i != outputIndex + 1){
                newArgs[j++] = tokens->items[i];
            }
        }
        newArgs[j] = NULL; //null term

        //execute
        execv(tokens->items[0], newArgs);
    } else{
        close(ifd);
        close(ofd);
        waitpid(pid, NULL, 0);
    }
}

void redirect(tokenlist *tokens){
	for(int i = 0; i < tokens->size; i++){
        if(strcmp(tokens->items[i], ">") == 0){
			if(tokens->items[i+2] != NULL && strcmp(tokens->items[i+2], "<") == 0){
				ioRedirect(tokens);
			} else{
				outputRedirect(tokens);
			}
			} else if(strcmp(tokens->items[i], "<") == 0){
				if(tokens->items[i+2] != NULL && strcmp(tokens->items[i+2], ">") == 0){
					ioRedirect(tokens);
			} else{
				inputRedirect(tokens);
			}
		}
	}
}

void backgroundProcessing(tokenlist *tokens, char *cmd){
    static int jobNumber = 1; //counts num background processes
    bool background = false; // flag

    if (tokens->size > 0 && strcmp(tokens->items[tokens->size - 1], "&") == 0){
        background = true; //set flag to true
        free(tokens->items[tokens->size - 1]);   //remove & from memory and null it
        tokens->items[tokens->size - 1] = NULL;
        tokens->size--;
    }

    pid_t pid = fork(); //fork!

    if (pid == -1){
		printf("Fork failed");
		exit(EXIT_FAILURE);
    } else if (pid == 0){
        if (execv(cmd, tokens->items) == -1){
            printf("execv failed");
            exit(EXIT_FAILURE);
        }
    } else{
        if (background == true){
            if (bgProcessCt < MAX_BIG_PROCS){ //update globals accordingly
                backgroundPIDs[bgProcessCt] = pid;
                backgroundCommands[bgProcessCt] = my_strdup(cmd);
                backgroundJobNumbers[bgProcessCt] = jobNumber;
                printf("[%d] %d\n", jobNumber, pid); //spit out job id etc.
                jobNumber++;
                bgProcessCt++; 
            }
        } else{
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

void updateBackground(){
    int status;
    for (int i = 0; i < bgProcessCt; i++){
        if (waitpid(backgroundPIDs[i], &status, WNOHANG) > 0){
            printf("[%d] + done %s\n", backgroundJobNumbers[i], 
            backgroundCommands[i] ? backgroundCommands[i] : "Unknown Command");
            free(backgroundCommands[i]);  // Free the command
            backgroundCommands[i] = NULL; // Avoid use-after-free
            backgroundPIDs[i] = -1;       // Mark as completed
        }
    }

    int j = 0;
    for (int i = 0; i < bgProcessCt; i++){
        if (backgroundPIDs[i] != -1){ //process still active
            if (i != j){
                backgroundPIDs[j] = backgroundPIDs[i];
                backgroundCommands[j] = backgroundCommands[i];
                backgroundJobNumbers[j] = backgroundJobNumbers[i]; //keep if job numbers are unique
            }
            j++;
        } else{
            // Free and NULL any commands that are left to ensure clean state
            if (backgroundCommands[i]){
                free(backgroundCommands[i]);
                backgroundCommands[i] = NULL;
            }
        }
    }
    bgProcessCt = j; // Update the count of active background processes
}
