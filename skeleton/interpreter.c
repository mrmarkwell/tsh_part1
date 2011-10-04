/***************************************************************************
 *  Title: Input 
 * -------------------------------------------------------------------------
 *    Purpose: Handles the input from stdin
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.4 $
 *    Last Modification: $Date: 2009/10/12 20:50:12 $
 *    File: $RCSfile: interpreter.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: interpreter.c,v $
 *    Revision 1.4  2009/10/12 20:50:12  jot836
 *    Commented tsh C files
 *
 *    Revision 1.3  2009/10/11 04:45:50  npb853
 *    Changing the identation of the project to be GNU.
 *
 *    Revision 1.2  2008/10/10 00:12:09  jot836
 *    JSO added simple command line parser to interpreter.c. It's not pretty
 *    but it works. Handles quoted strings, and preserves backslash behavior
 *    of bash. Also, added simple skeleton code as well as code to create and
 *    free commandT structs given a command line.
 *
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.4  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.3  2002/10/21 04:47:05  sempi
 *    Milestone 2 beta
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __INTERPRETER_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <unistd.h>
#include "string.h"

/************Private include**********************************************/
#include "interpreter.h"
#include "io.h"
#include "runtime.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */
typedef struct string_l
{
  char* s;
  struct string_l* next;
} stringL;

int BUFSIZE = 512;
int MAXARGS = 100;

/**************Function Prototypes******************************************/

commandT*
getCommand(char* cmdLine);

void
freeCommand(commandT* cmd);

int
doesFileExist(const char * name);
/**************Implementation***********************************************/

/* 
 * doesFileExist
 *
 * arguments:
 *   char * name: the name of the command given
 *
 *   returns: bool, true if the file exists
 *
 *   This is a function that opens and closes a file to check if it 
 *   exists.
 */

int doesFileExist(const char * name) {
  FILE * file;
  if ((file = fopen(name, "r"))) {
    fclose(file);
    return TRUE; // file exists
  }
  return FALSE; // file doesn't exist
} /* doesFileExist */

/*
 * Interpret
 *
 * arguments:
 *   char *cmdLine: pointer to the command line string
 *
 * returns: none
 *
 * This is the high-level function called by tsh's main to interpret a
 * command line.
 */
void
Interpret(char* cmdLine)
{
  // int i = 0;
  commandT* cmd = getCommand(cmdLine);
  char * pathlist = getenv("PATH");
  char * home = getenv("HOME");
  char * homeCopy = malloc(MAXPATHLEN*sizeof(char*));
  strcpy(homeCopy,home);
  char * pathCopy = malloc(MAXPATHLEN*sizeof(char*));
  strcpy(pathCopy,pathlist);

  char * current = getCurrentWorkingDir();
  // printf("argc: %d\n", cmd->argc);
  // for (i = 0; cmd->argv[i] != 0; i++)
  //   {
  //     printf("#%d|%s|\n", i, cmd->argv[i]);
  //   }
  strcat(homeCopy,"/");
  strcat(homeCopy,cmd->name);
  if (cmd->name[0] == '/') {
    if (doesFileExist(cmd->name)) {
      printf("The absolute path is %s\n\n",cmd->name);
    }
  } else {

      if (doesFileExist(homeCopy)) {
        printf("Found in the home directory, abs path: %s\n\n",homeCopy);
      }  else {
        strcat(current,"/");
        strcat(current,cmd->name);
        if (doesFileExist(current)) {
          printf("The file is in the current directory, path: %s\n\n",current);
        } else {

          char* fullpath = strtok(pathCopy, ":");
            while (fullpath != NULL) {
              char * path = malloc(2*(sizeof(fullpath) + sizeof(cmd->name)));
              strcpy(path,fullpath);
              strcat(path,"/");
              if (doesFileExist(strcat(path,cmd->name))) {
                printf("The file exists at %s\n",path);
              } 
              fullpath = strtok(NULL, ":");
              free(path);
            }
          }
      }
  }
free(current);
free(pathCopy);
free(homeCopy);
freeCommand(cmd);
  
} /* Interpret */


/*
 * getCommand
 *
 * arguments:
 *   char *cmdLine: pointer to the command line string
 *
 * returns: commandT*: pointer to the commandT struct generated by
 *                     parsing the cmdLine string
 *
 * This parses the command line string, and returns a commandT struct,
 * as defined in runtime.h.  You must free the memory used by commandT
 * using the freeCommand function after you are finished.
 *
 * This function tokenizes the input, preserving quoted strings. It
 * supports escaping quotes and the escape character, '\'.
 */
commandT*
getCommand(char* cmdLine)
{
  commandT* cmd = malloc(sizeof(commandT) + sizeof(char*) * MAXARGS);
  cmd->argv[0] = 0;
  cmd->name = 0;
  cmd->argc = 0;

  int i, inArg = 0;
  char quote = 0;
  char escape = 0;

  // Set up the initial empty argument
  char* tmp = malloc(sizeof(char*) * BUFSIZE);
  int tmpLen = 0;
  tmp[0] = 0;

  //printf("parsing:%s\n", cmdLine);

  for (i = 0; cmdLine[i] != 0; i++)
    {
      //printf("\tindex %d, char %c\n", i, cmdLine[i]);

      // Check for whitespace
      if (cmdLine[i] == ' ')
        {
          if (inArg == 0)
            continue;
          if (quote == 0)
            {
              // End of an argument
              //printf("\t\tend of argument %d, got:%s\n", cmd.argc, tmp);
              cmd->argv[cmd->argc] = malloc(sizeof(char) * (tmpLen + 1));
              strcpy(cmd->argv[cmd->argc], tmp);

              inArg = 0;
              tmp[0] = 0;
              tmpLen = 0;
              cmd->argc++;
              cmd->argv[cmd->argc] = 0;
              continue;
            }
        }

      // If we get here, we're in text or a quoted string
      inArg = 1;

      // Start or end quoting.
      if (cmdLine[i] == '\'' || cmdLine[i] == '"')
        {
          if (escape != 0 && quote != 0 && cmdLine[i] == quote)
            {
              // Escaped quote. Add it to the argument.
              tmp[tmpLen++] = cmdLine[i];
              tmp[tmpLen] = 0;
              escape = 0;
              continue;
            }

          if (quote == 0)
            {
              //printf("\t\tstarting quote around %c\n", cmdLine[i]);
              quote = cmdLine[i];
              continue;
            }
          else
            {
              if (cmdLine[i] == quote)
                {
                  //printf("\t\tfound end quote %c\n", quote);
                  quote = 0;
                  continue;
                }
            }
        }

      // Handle escape character repeat
      if (cmdLine[i] == '\\' && escape == '\\')
        {
          escape = 0;
          tmp[tmpLen++] = '\\';
          tmp[tmpLen] = 0;
          continue;
        }

      // Handle single escape character followed by a non-backslash or quote character
      if (escape == '\\')
        {
          if (quote != 0)
            {
              tmp[tmpLen++] = '\\';
              tmp[tmpLen] = 0;
            }
          escape = 0;
        }

      // Set the escape flag if we have a new escape character sequence.
      if (cmdLine[i] == '\\')
        {
          escape = '\\';
          continue;
        }

      tmp[tmpLen++] = cmdLine[i];
      tmp[tmpLen] = 0;
    }
  // End the final argument, if any.
  if (tmpLen > 0)
    {
      //printf("\t\tend of argument %d, got:%s\n", cmd.argc, tmp);
      cmd->argv[cmd->argc] = malloc(sizeof(char) * (tmpLen + 1));
      strcpy(cmd->argv[cmd->argc], tmp);

      inArg = 0;
      tmp[0] = 0;
      tmpLen = 0;
      cmd->argc++;
      cmd->argv[cmd->argc] = 0;
    }

  free(tmp);

  cmd->name = cmd->argv[0];

  return cmd;
} /* getCommand */


/*
 * freeCommand
 *
 * arguments:
 *   commandT *cmd: pointer to the commandT struct to be freed
 *
 * returns: none
 *
 * This function frees all the memory associated with the given
 * commandT struct, before freeing the struct's memory itself.
 */
void
freeCommand(commandT* cmd)
{
  int i;

  cmd->name = 0;
  for (i = 0; cmd->argv[i] != 0; i++)
    {
      free(cmd->argv[i]);
      cmd->argv[i] = 0;
    }
  free(cmd);
} /* freeCommand */
