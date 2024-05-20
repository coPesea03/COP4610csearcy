# Shell Project

The purpose of this project is to design and develop a comprehensive shell interface that enhances process control, user interaction, and error handling mechanisms. By constructing this shell, you will gain valuable insights into the intricacies of operating system processes, command-line parsing, and the robustness required for error-free execution.

By successfully completing this project, you will not only gain practical experience in process control, command-line parsing, and error handling, but also have the opportunity to build a robust and user-friendly shell interface that empowers users to efficiently interact with the operating system.

## Group Information

- **Group Number:** 28
- **Group Name:** Group 28
- **Group Members:**
  - Connor Searcy: cps21a@fsu.edu
  - Michael Clark: mtc21c@fsu.edu
  - Cesar Marquez: Cdm21h@fsu.edu


## Division of Labor:

### Part 1: Prompt

Responsibilities: The user will be greeted with a prompt that should indicate the absolute working directory, the user name, and the machine name. You can do this by expanding the $USER, $MACHINE, $PWD environment variables. The user will type on the same line as the prompt.

USER@MACHINE:PWD>

Completed by: Michael Clark, Cesar Marquez, Connor Searcy

### Part 2: Environment Variables

Responsibilities: We had to implement way to replacing tokens beginning with $ with the correct environment variable. For example, given command ["echo $HOME"] the shell will print ["echo clark"]. This will apply universally throughout the shell.

Completed By: Cesar Marquez, Connor Searcy

### Part 3: Tilde Expansion

Responsibilities: In this part we had to implement a way to replace the ~ symbol with the $HOME environment variable. For example, ["ls ~/bin"] will be expanded to ["ls /home/clark/bin"]. We only had to implement ways to deal with the case of a standalone ~ or if it begins with ~/dir.

Completed By: Cesar Marquez, Connor Searcy

### Part 4: $PATH Search

Responsibilities: In order to perform execution we first need to obtain the proper $PATH for the command. This is done by searching through the $PATH environment variable with the command appended to the end of each path directory. In this part we implement a way to perform the $PATH search to find the proper $PATH before proceeding to execution. This is done by delimiting the $PATH variable with a colon and examining each part. If the $PATH is not found an error message will be displayed.

Completed By: Connor Searcy and Michael Clark

### Part 5: External Command Execution

Responsibilities: In this part we take the $PATH found in part 4 and execute the command. This is done by using fork() to create a child process and calling execv() to execute the command within the child process. We will make the execution able to handle all external commands with flags. An example of an external command to be executed is ["ls -al"].

Completed By: Michael Clark, Cesar Marquez, Connor Searcy

### Part 6: I/O Redirection

Responsibilities: In this part we will implement a way to handle I/O redirection within our shell following these guidelines:
```plaintext
            cmd > file_out
            - will write standard output to the output file
            - if a file does not exist a new file will be created
            - if a file already exists it will be overwritten

            cmd < file_in
            - cmd receives its standard input from file_in
            - an error message will be signaled if file does not exist

            cmd < file_in > file_out
            - cmd recieves its standard input from file_in
            - cmd writes its standard output to file_out
```
Some additional rules will be:
Our processes will not modify the contents of the input file whatsoever
Output redirection should create a new file with the following file permissions -rw-------
Output redirection should overwrite(not append) with the same name and same permissions listed above

Completed By: Michael Clark and Connor Searcy

### Part 7: Piping

Responsibilities: [Description] Completed By: n/a

### Part 8: Background Processing

Responsibilities: The final functionality we will incorporate is background processing. Thus far, the shell has been waiting to prompt for additional user input whenever there were external commands running. Background processing allows the shell to execute external commands without waiting for their completion. However, it is still essential for the shell to keep track of their completion status periodically.

It's worth noting that background processing should seamlessly integrate with I/O redirection and piping functionalities. This means that background processing can be used in conjunction with I/O redirection or within command pipelines.

Background processing behavior should adhere to the following guidelines:
```plaintext
cmd &
Execute cmd in the background.
Upon execution start, print [Job number] [cmd's PID].
Upon completion, print [Job number] + done [cmd's command line].
```
Background processing also supports redirection functionalities:
```plaintext
cmd > file &
cmd writes its standard output to file in the background.
cmd < file &
cmd receives its standard input from file in the background.
cmd < file_in > file_out &
cmd receives its standard input from file_in and writes its standard output to file_out in the background.
```
Additionally, all background processes executed by the shell must be kept track of with a relative job number starting from 1 and incrementing so forth. Job numbers will not be reused. You can also assume that there will not be more than 10 background processes running concurrently.

Completed By: Connor Searcy

### Part 9: Internal Command Execution

Responsibilities: In this part we implement the internal commands of the shell. This is done by implementing function inside the shell for each command. The internal commands for the shell will be: exit, cd PATH, jobs.

                  exit:
                   - Will wait for any background processes that are running to finish
                   - display the last three valid commands, if less than three valid commands print the last valid one
                   - If there are no valid commands say so
                   
                  cd PATH:
                   - Changes the current working directory
                   - if no arguments are supplied, change the current working directory to $HOME
                   - signal an error if more than one argument is present
                   - signal an error if the target does not exist

                  jobs:
                   - outputs a list of active background processes
                   - if there are no active background processes say so
                   - format, [Job number]+[CMD's PID]+[CMD's command line]

Completed By: Connor Searcy and Michael Clark
                           
### Extra Credit

Responsibilities: [Description] Completed By: n/a


## File Listing:

```plaintext
project-1-os-group-28/
├── shell/
│   ├── include/
│   │   ├── executor.h
│   │   ├── lexer.h
│   │   ├── parser.h
│   ├── src/
│   │   ├── executor.c
│   │   ├── lexer.c
│   │   ├── main.c
│   │   ├── parser.c
│   ├── Makefile
├── README.md
```
## How to Compile & Execute:

```bash
git clone https://github.com/FSU-COP4610-S24/project-1-os-group-28.git
```
Compile the program on linprog using the provided Makefile. To execute, run the executable named "shell" created by the makefile.
```bash
make run
```
This will build the executable at: /bin/shell

To run with gdb or valgrind, navigate to the /shell/bin directory and run 
```bash
gdb shell
```
or
```bash
valgrind shell
```

## Requirements: 
### Compiler: 
gcc compiler 
### Dependencies: 
n/a

## Bugs

1. When printing the last 3 valid commands, the counter counts backwards.
2. When running a command that expects an argument (such as cat {FILENAME}), the program stops working. So like dont do that 
3. I am not sure this classifies as a bug, but i ran wc -L (file) like you said to in the Canvas announcement, and one of my files is 144, according to the command. However, i just triple checked for whitespace, long lines, etc, and my longest line is 100 characters. I have a line of code that is separated by a "," on two separate lines, and i think that could be why the command outputs that. Please do not deduct points for this! Thanks!

## Extra Credit

Extra Credit 1: N/A Extra Credit 2: N/A Extra Credit 3: N/A

## Considerations

Unfortunately, we were unable to solve the Piping problem. We, in hindsight, started too late. When the semester is over (or when we get a grade) Connor plans to solve the piping problem for himself.

Question for the grader/professor: Am I allowed to use this code on a personal showcase GitHub? I would like to put it on my LinkedIn as a completed project. If not, I can make a video of myself testing it instead. I understand academic integrity standards!
