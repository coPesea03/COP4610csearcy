#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

char *my_strdup(const char*);               //idk why we had to do this, read comment in parser.c
void replaceToken(tokenlist*, int, char*);  //these functions are used to remove ~ or $ as a token 
void expandSpecialTokens(tokenlist*);       //and properly handle these tokens as their env values

char *CmdApp(tokenlist*);                   //handles cases where first token is an internal 
                                            //command, finds command in the $PATH
