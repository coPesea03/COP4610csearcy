#include "lexer.h"
#include "parser.h"

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

//part 5 function
void execute(tokenlist *tokens);    //this is the main function for executing commands w/ path

//part 6 functions
void redirect(tokenlist *tokens);   //this function houses if/then logic for handling ioRedirect
void outputRedirect(tokenlist *tokens);     //handles output redirection
void inputRedirect(tokenlist *tokens);      //handles input redirection
void ioRedirect(tokenlist *tokens);         //handles cmd < infile > outfile and opposite

//part 8 functions and struct:
void backgroundProcessing(tokenlist *, char *);
void updateBackground();

typedef struct {
    pid_t pid;        // Process ID
    char* command;    // Command executed by the process
    int jobNumber;    // Job number for user reference
} BackgroundProcess;

//part 9 functions
void handleExit(tokenlist *tokens);         //handles the case that the token is "exit"
void updateCommandHistory(char* command);   //handles history of commands for "exit"
void handleCDPATH(tokenlist *tokens);       //handles the case that the token is "cd [PATH]"
void handleJobs();         //handles the case that the token is "jobs"