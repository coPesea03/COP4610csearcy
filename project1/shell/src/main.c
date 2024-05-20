#include "parser.h"
#include "lexer.h"
#include "executor.h"

#include "string.h"

int main(){

   while(1){
      updateBackground();

      char cwd[200]; //this is used to store the current working directory to update prompt
      getcwd(cwd, sizeof(cwd));      
      printf("%s@%s:%s> ", getenv("USER"), getenv("MACHINE"), cwd); // print the prompt   
      
      tokenlist *tokens = shell();   //init instance of the shell
      
      if(tokens == NULL || tokens->size == 0){  //checks for input, if none, send notice 
         printf("No input detected\n");         //and free memory (avoids seg fault)
         free_tokens(tokens);
         continue;
      }

      char *part9[] = {"exit", "cd", "jobs"}; //init hard coded array to look for part 9 key words
      //in hindsight, this array is kind of useless and i couldve just put the strings, but i 
      //dont rly feel like changing it

      expandSpecialTokens(tokens); //expands ~ and env. variables if used as arguments in a command

      //this if/else block searches tokens for special characters/key terms and then sends the 
      //system off to handle those cases. if not any of the special cases, it either runs a command 
      //or error checks                            //each of the next 3 else if's activate pt 9
      if(0 == strcmp(tokens->items[0], part9[0])){ //calls handleExit()
         handleExit(tokens);
      } else if(0 == strcmp(tokens->items[0], part9[1])){ //calls handleCDPATH()
         handleCDPATH(tokens);
         updateCommandHistory(tokens->items[0]);
      } else if(0 == strcmp(tokens->items[0], part9[2])){ //calls handleJobs()
         handleJobs();
         updateCommandHistory(tokens->items[0]);
      } else if(tokens->size >= 1){       //part6 activation
         char *validCmd = CmdApp(tokens); //sets command w/ path as a char*
         bool keepGoing = true;           //used to break for loop below
         if(validCmd != NULL){
         updateCommandHistory(tokens->items[0]);
            for(int i = 0; i < tokens->size; i++){    //searches for > or <
               if(strcmp(tokens->items[i], ">") ==0 || strcmp(tokens->items[i], "<") == 0){ 
                     free(tokens->items[0]); //if found, open up tokens->items[1] to be allocated 
                     tokens->items[0] = malloc(strlen(validCmd) + 1);//to be validCmd + null buffer
                     strcpy(tokens->items[0], validCmd);
                     redirect(tokens); //calls redirect function in executor.c
                     keepGoing = false; //only reaches this statement if > or < is found. 
                     break;         //if it isnt, it will remain true and run without the 
                  }                 //file redirection
               } 
            //this code is the same as above except it calls execute() after allocating the memory.
            //only triggers if there is no < or > found 
            if(keepGoing ==true){
               for(int i = 0; i < tokens->size; i++){
                  if(strcmp(tokens->items[i], "&") == 0){
                        //triggers part 8
                        backgroundProcessing(tokens, validCmd); 
                        keepGoing = false;
                        break;
                     } else{
                        keepGoing = true;
                        continue;
                     }
               }
            }
            if(keepGoing == true){      //this code is the same as above except it calls execute() 
               free(tokens->items[0]);  //only triggers if there is no < or > found, or &
               tokens->items[0] = malloc(strlen(validCmd) + 1);
               strcpy(tokens->items[0], validCmd);
               execute(tokens);
               free(validCmd);
               }
         } else{               //prints error if validCmd is null
            fprintf(stderr, "Command not found\n");
      }
   }
   free_tokens(tokens); 

}
}
