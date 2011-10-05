/***************************************************************************
 *  Title: Runtime environment 
 * -------------------------------------------------------------------------
 *    Purpose: Runs commands
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.3 $
 *    Last Modification: $Date: 2009/10/12 20:50:12 $
 *    File: $RCSfile: runtime.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: runtime.c,v $
 *    Revision 1.3  2009/10/12 20:50:12  jot836
 *    Commented tsh C files
 *
 *    Revision 1.2  2009/10/11 04:45:50  npb853
 *    Changing the identation of the project to be GNU.
 *
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.6  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.5  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.4  2002/10/21 04:49:35  sempi
 *    minor correction
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
#define __RUNTIME_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

/************Private include**********************************************/
#include "runtime.h"
#include "io.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/

#define NBUILTINCOMMANDS (sizeof BuiltInCommands / sizeof(char*))

typedef struct bgjob_l
{
  pid_t pid;
  struct bgjob_l* next;
} bgjobL;

/* the pids of the background processes */
bgjobL *bgjobs = NULL;

/************Function Prototypes******************************************/
/* run command */
static void
RunCmdFork(commandT*, bool);
/* runs an external program command after some checks */
static void
RunExternalCmd(commandT*, bool);
/* resolves the path and checks for exutable flag */
static bool
ResolveExternalCmd(commandT*);
/* forks and runs a external program */
static void
Exec(commandT*, bool);
/* runs a builtin command */
static void
RunBuiltInCmd(commandT*);
/* checks whether a command is a builtin command */
static bool
IsBuiltIn(char*);
/* finds the full path of a given name */
char * 
getFullPath(char * name);
/* checks if the file does exist at path name */
int 
doesFileExist(const char * name);
/* changes argv[0] to contain only the file name */
void
argZeroConverter(commandT* cmd);
/************External Declaration*****************************************/

/**************Implementation***********************************************/


/*
 * RunCmd
 *
 * arguments:
 *   commandT *cmd: the command to be run
 *
 * returns: none
 *
 * Runs the given command.
 */
void
RunCmd(commandT* cmd)
{
  RunCmdFork(cmd, TRUE);
} /* RunCmd */


/*
 * RunCmdFork
 *
 * arguments:
 *   commandT *cmd: the command to be run
 *   bool fork: whether to fork
 *
 * returns: none
 *
 * Runs a command, switching between built-in and external mode
 * depending on cmd->argv[0].
 */
void
RunCmdFork(commandT* cmd, bool fork)
{
  if (cmd->argc <= 0)
    return;
  if (IsBuiltIn(cmd->argv[0]))
    {
      RunBuiltInCmd(cmd);
    }
  else
    {
      RunExternalCmd(cmd, fork);
    }
} /* RunCmdFork */


/*
 * RunCmdBg
 *
 * arguments:
 *   commandT *cmd: the command to be run
 *
 * returns: none
 *
 * Runs a command in the background.
 */
void
RunCmdBg(commandT* cmd)
{
  // TODO
} /* RunCmdBg */


/*
 * RunCmdPipe
 *
 * arguments:
 *   commandT *cmd1: the commandT struct for the left hand side of the pipe
 *   commandT *cmd2: the commandT struct for the right hand side of the pipe
 *
 * returns: none
 *
 * Runs two commands, redirecting standard output from the first to
 * standard input on the second.
 */
void
RunCmdPipe(commandT* cmd1, commandT* cmd2)
{
} /* RunCmdPipe */


/*
 * RunCmdRedirOut
 *
 * arguments:
 *   commandT *cmd: the command to be run
 *   char *file: the file to be used for standard output
 *
 * returns: none
 *
 * Runs a command, redirecting standard output to a file.
 */
void
RunCmdRedirOut(commandT* cmd, char* file)
{
} /* RunCmdRedirOut */


/*
 * RunCmdRedirIn
 *
 * arguments:
 *   commandT *cmd: the command to be run
 *   char *file: the file to be used for standard input
 *
 * returns: none
 *
 * Runs a command, redirecting a file to standard input.
 */
void
RunCmdRedirIn(commandT* cmd, char* file)
{
}  /* RunCmdRedirIn */


/*
 * RunExternalCmd
 *
 * arguments:
 *   commandT *cmd: the command to be run
 *   bool fork: whether to fork
 *
 * returns: none
 *
 * Tries to run an external command.
 */
static void
RunExternalCmd(commandT* cmd, bool fork)
{
  if (ResolveExternalCmd(cmd)) {
    Exec(cmd, fork);
  }
}  /* RunExternalCmd */


/*
 * ResolveExternalCmd
 *
 * arguments:
 *   commandT *cmd: the command to be run
 *
 * returns: bool: whether the given command exists
 *
 * Determines whether the command to be run actually exists.
 */
static bool
ResolveExternalCmd(commandT* cmd)
{
  char* rootpath = getFullPath(cmd->name);
  if (rootpath != NULL) {
    cmd->name = rootpath;
    return TRUE;
  }
  return FALSE;
} /* ResolveExternalCmd */


/*
 * Exec
 *
 * arguments:
 *   commandT *cmd: the command to be run
 *   bool forceFork: whether to fork
 *
 * returns: none
 *
 * Executes a command.
 */
static void
Exec(commandT* cmd, bool forceFork)
{
  
  if (forceFork) { // Do this if you should fork
  int pid;
  sigset_t x;
  sigemptyset (&x);
  sigaddset(&x,SIGCHLD);
  if (sigprocmask(SIG_BLOCK,&x,NULL) != 0) {
    PrintPError("Signal Block Failure");
  }
    if ((pid = fork()) < 0) { // fork returns negative if it fails.
    PrintPError("Fork failed");
  } else {
      if (pid == 0) { // Child - to exec
        setpgid(0,0); // remove from foreground process group
        argZeroConverter(cmd);
        sigprocmask(SIG_UNBLOCK,&x,NULL);
        execv(cmd->name,cmd->argv);
          PrintPError("Execv failed");
      }
      else {  // Parent - wait for child to be reaped.
        fgpid = pid; // foreground process id
        int * status = malloc(sizeof(int));
        int * freethis = status;
        waitpid(pid,status,0);
        fgpid = 0;
        free(freethis);
      }
  }
  free(cmd->name);
  }
} /* Exec */

/*
 * argZeroConverter
 *
 * This function takes a commandT* struct and makes the 
 * argv[0] position contain only the file name and 
 * nothing more.
 *
 * It returns void
 */

void
argZeroConverter(commandT* cmd) {
  char* current = cmd->argv[0];
  int slash = -1;
  int i;
  for (i = strlen(current); i >= 0; --i) {
    if (current[i] == '/') {
      slash = i;  // find the farthest right slash
      break;
    }
  }
  if (slash != -1) { // if there is a slash, put everything after into argv[0]
    char* command = malloc((strlen(current) - slash + 1) * sizeof(char));
    memcpy(command, current + (slash + 1) * sizeof(char), (strlen(current) - slash + 1) * sizeof(char));
    free(cmd->argv[0]);
    cmd->argv[0] = command;
  }
} /* argZeroConverter */
/*
 * IsBuiltIn
 *
 * arguments:
 *   char *cmd: a command string (e.g. the first token of the command line)
 *
 * returns: bool: TRUE if the command string corresponds to a built-in
 *                command, else FALSE.
 *
 * Checks whether the given string corresponds to a supported built-in
 * command.
 */
static bool
IsBuiltIn(char* cmd)
{
 if ( strcmp(cmd,"echo") == 0 ||
    strcmp(cmd,"cd") == 0 ||
    strcmp(cmd,"exit") == 0) {
   return TRUE;
 }  
  return FALSE;
} /* IsBuiltIn */


/*
 * RunBuiltInCmd
 *
 * arguments:
 *   commandT *cmd: the command to be run
 *
 * returns: none
 *
 * Runs a built-in command.
 */
static void
RunBuiltInCmd(commandT* cmd)
{
  if (strcmp(cmd->argv[0],"echo") == 0) { // runs command echo
    int i;
    for(i = 1; i < cmd->argc; i++) {
      printf("%s ",cmd->argv[i]);
    }
    PrintNewline();
  }

  if (strcmp(cmd->argv[0],"cd") == 0) { // runs command cd
    int dir = 0;
    if (cmd->argc > 1) {
    dir = chdir(cmd->argv[1]);
    } else {
      dir = chdir(getenv("HOME"));
    }
      //0 if success, -1 if failure
  if (dir != 0) {
    PrintPError("cd error");
  }
  }
  if (strcmp(cmd->argv[0],"exit") == 0) { // escapes if command is exit
    return;
  }

} /* RunBuiltInCmd */


/*
 * CheckJobs
 *
 * arguments: none
 *
 * returns: none
 *
 * Checks the status of running jobs.
 */
void
CheckJobs()
{
} /* CheckJobs */

/*
 * getCurrentWorkingDir
 *
 * Takes no arguments, this function uses the getcwd funcion
 * to return the path to the current working directory
 *
 * Returns path to current working directory
 *
 */
char *
getCurrentWorkingDir() {
  char * path = malloc(MAXPATHLEN*sizeof(char*));
  return getcwd(path,MAXPATHLEN);
} /* getCurrentWorkingDir */


/*
 * getFullPath
 *
 * This function takes the cmd->name and returns
 * the full path to the file specified in cmd->name
 *
 * It returns a pointer to NULL if it failed to find the file.
 *
 */

char *
getFullPath(char * name) {
  bool found = FALSE;
  char * pathlist = getenv("PATH"); // prepare the memory for the possible paths
  char * home = getenv("HOME");
  char * homeCopy = malloc(MAXPATHLEN*sizeof(char*));
  strcpy(homeCopy,home);
  char * pathCopy = malloc(MAXPATHLEN*sizeof(char*));
  strcpy(pathCopy,pathlist);
  char * result = malloc(MAXPATHLEN*sizeof(char*)); // prepare memory to store the result
  char * current = getCurrentWorkingDir();
  // printf("argc: %d\n", cmd->argc);
  // for (i = 0; cmd->argv[i] != 0; i++)
  //   {
  //     printf("#%d|%s|\n", i, cmd->argv[i]);
  //   }
  strcat(homeCopy,"/");
  strcat(homeCopy,name);
  if (name[0] == '/') { // if it is an absolute path, store result.
    if (doesFileExist(name)) {
      strcpy(result,name);
      found = TRUE;
    }
  } else {
      if (doesFileExist(homeCopy)) { // If it is in the home directory
        strcpy(result,homeCopy);
        found = TRUE;
      }  else {
        strcat(current,"/");
        strcat(current,name);
        if (doesFileExist(current)) { // If it is in the current directory
          strcpy(result,current);
          found = TRUE;
        } else { // Else, check every path in PATH environment variable
          char* fullpath = strtok(pathCopy, ":");
            while (fullpath != NULL) {
              char * path = malloc(MAXPATHLEN*sizeof(char*));
              strcpy(path,fullpath);
              strcat(path,"/");
              if (doesFileExist(strcat(path,name))) {
                strcpy(result,path);
                found = TRUE;
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
if (found) {
  return result;
} else {
  PrintPError("Unable to locate file");
  return NULL;
}
} /* getFullPath */



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

