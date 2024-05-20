#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


char *my_strdup(const char *s){        //Ran into an error where strdup() wouldnt work 
   char *copy = malloc(strlen(s) + 1); //with -std-99 flag Honestly, not sure why. 
                                       //But we just remade a simple verison of it and use it later
   if(copy != NULL){                   
      strcpy(copy, s);
   }

   return copy;
} 

void replaceToken(tokenlist *tokens, int index, char *newValue){
   free(tokens->items[index]);                  //frees the original token and
   tokens->items[index] = my_strdup(newValue);  //puts the new value into the token array
}

void expandSpecialTokens(tokenlist *tokens){
   for (int i = 0; i < tokens->size; i++){
      char *token = tokens->items[i];
      if (token[0] == '$' && strlen(token) > 1){ //env variable expansion
         char *envValue = getenv(token + 1);
         
         if (envValue != NULL){ 
            replaceToken(tokens, i, envValue);
         }

      } else if (token[0] == '~'){ //tilda expansion
         char *homePath = getenv("HOME");
         if (homePath != NULL){
            if (strlen(token) > 1){ //checks if there's more after ~
               char *fullPath = malloc(strlen(homePath) + strlen(token));
               if (fullPath){
                  strcpy(fullPath, homePath); //puts home directory+token w/ null term in fullPath
                  strcat(fullPath, token + 1); 
                  replaceToken(tokens, i, fullPath);
                  free(fullPath); //sends fullPath to replaceToken and deallocates
                  }
               } else{//handles case of just ~
                  replaceToken(tokens, i, homePath);
            }
         }
      }
   }
}


// function handles cases where first token is an internal command, finds command in the $PATH
char *CmdApp(tokenlist *tokens){ 
   char *pathValue = getenv("PATH");   //gets $PATH
   char *slash = "/";                  //gets a / (the path needs to be $PATH/cmd, this adds the /)
   char *token = tokens->items[0];     //gets the command value

   if(pathValue != NULL){
      char *pathCopy = my_strdup(pathValue);  //uses strdup() to copy in the $PATH to be tokenized
      if(pathCopy != NULL){
         char *dir = strtok(pathCopy, ":");   //tokenized version of the path, delimiter is :

         while(dir != NULL){ //allocates memory for tokenized list of $PATH, a slash, length of 
            char *cmd = malloc(strlen(dir) + 1 + strlen(token) + 1);   //command, and a nullbuf
            strcpy(cmd, dir);          //these 3 commands glue together the command with path
            strcat(cmd, slash);
            strcat(cmd, token);

				if(access(cmd, F_OK) == 0){ //if the command is found, free the memory and return the
               free(pathCopy);          // command with path
               return cmd;
				} else{                    //free the malloc for cmd, it is now defunct.
               free(cmd);
            }               
            dir = strtok(NULL, ":");    //deallocate the tokenized list
         }
         free(pathCopy);
      } else{                           //if pathCopy does not exist
         fprintf(stderr, "Memory allocation failed\n");
         return NULL;
      }
   } else{                              //if pathValue does not exist
      printf("Path not set\n");
      return NULL;
   }
   return NULL; //we kept getting an error that a function expecting to end with a return. 
                //we just return null and null check in main, it should never be null, though
}
