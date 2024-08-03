#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <fcntl.h>
#include "builtin.h"
#include "parse.h"
#include <sys/wait.h>

/*******************************************
 * Set to 1 to view the command line parse *
 *******************************************/
#define DEBUG_PARSE 0


void print_banner ()
{
    printf ("                    ________   \n");
    printf ("_________________________  /_  \n");
    printf ("___  __ \\_  ___/_  ___/_  __ \\ \n");
    printf ("__  /_/ /(__  )_(__  )_  / / / \n");
    printf ("_  .___//____/ /____/ /_/ /_/  \n");
    printf ("/_/ Type 'exit' or ctrl+c to quit\n\n");
}


/* returns a string for building the prompt
 *
 * Note:
 *   If you modify this function to return a string on the heap,
 *   be sure to free() it later when appropirate!  */
static char* build_prompt ()
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL){
      fprintf(stdout,"%s",cwd);
    }
    return  "$ ";
    
}


/* return true if command is found, either:
 *   - a valid fully qualified path was supplied to an existing file
 *   - the executable file was found in the system's PATH
 * false is returned otherwise */
static int command_found (const char* cmd)
{
    char* dir;
    char* tmp;
    char* PATH;
    char* state;
    char probe[PATH_MAX];

    int ret = 0;

    //Can I execute the command you supplied
    if (access (cmd, X_OK) == 0)
        return 1;

    //Getting $Path Enviornment Variable
    PATH = strdup (getenv("PATH"));

    //Tokenize the directories
    for (tmp=PATH; ; tmp=NULL) {
        dir = strtok_r (tmp, ":", &state);
        if (!dir)
            break;

        //Checks whether cmd is in current path
        strncpy (probe, dir, PATH_MAX-1);
        strncat (probe, "/", PATH_MAX-1);
        strncat (probe, cmd, PATH_MAX-1);

        if (access (probe, X_OK) == 0) {
            ret = 1;
            //Here is where you'd return probe for which functionality
            break;
        }
    }

    free (PATH);
    return ret;
}


/* Called upon receiving a successful parse.
 * This function is responsible for cycling through the
 * tasks, and forking, executing, etc as necessary to get
 * the job done! */

void execute_tasks (Parse* P)
{
    int ntasks = P->ntasks;
    int fd[ntasks*2];     //Each task has a r&w side
    int fdi, fdo;         //File Descriptors for in/outfile
    unsigned int t;

    int stdout_cpy;
    int stdin_cpy;

    //Input Redirection
    if (P->infile){
      fdi = open(P->infile, O_RDONLY); //Opening Input File
      stdin_cpy = dup2(0,350); //Cpying Stdin
      if (stdin_cpy == -1){
        fprintf(stderr, "dup2() failed at input redirection!\n");
        exit(EXIT_FAILURE);
        }
      if (dup2(fdi, STDIN_FILENO) == -1) { //Changing input to infile
        fprintf(stderr, "dup2() failed at input redirection!\n");
        exit(EXIT_FAILURE);
        }
      close(fdi);
      }

      //Output redirection
      if (P->outfile){
        fdo = open(P->outfile, O_CREAT|O_WRONLY, S_IRWXU); //Opening the outfile
        stdout_cpy = dup2(1,351); //Copying @tdout
        if (stdout_cpy == -1){ 
          fprintf(stderr, "dup2() failed at output redirection!\n");
          exit(EXIT_FAILURE);
          }
        if (dup2(fdo, STDOUT_FILENO) == -1) { //Chanfing output to outfile
          fprintf(stderr, "dup2() failed at output redirection!\n");
          exit(EXIT_FAILURE);
          }
          close(fdo);
        }
 
    for (t = 0; t < P->ntasks; t++) { //Loops through all tasks
      //If Builtin Cmmd
      if (is_builtin (P->tasks[t].cmd)) {
        builtin_execute (P->tasks[t]);
        }

      //If not Builtin command but is in PATH
      else if (command_found (P->tasks[t].cmd)) {
        //Opening up pipe if there is more tasks to do
        if (((t+1) < P->ntasks) && (ntasks>1)){
          if (pipe((fd+(2*t))) == -1){
            fprintf(stderr, "failed to create pipe\n");
              return;
              }

        }
        switch (fork()) {
          case -1: //fork() error checking
            fprintf(stderr, "ERROR: failed to fork()\n");
            exit(EXIT_FAILURE);
    
          case 0: //child case
          
            //First Task
            if ((t==0) && (P->ntasks > 1)){ 
              close(fd[0]); //Closing Read Side
              if (dup2(fd[1], STDOUT_FILENO) == -1) {
                fprintf(stderr, "dup2() fialed!\n");
                exit(EXIT_FAILURE);
              }
              close(fd[1]); //Closing Write Side
            }

            //Last task
            else if (((t+1) == ntasks) && (ntasks > 1)){ 
              if (dup2(fd[2*t-2], STDIN_FILENO) == -1) {
                fprintf(stderr, "dup2() fialed!\n");
                exit(EXIT_FAILURE);
              }
                close(fd[2*t-2]); //Close all  open pipes
                }

            //Intermediate Tasks
            else if (((t+1) < ntasks) && (ntasks > 1)){ 
              //Close write side of current pipe
              close(fd[2*t]); 
              
              //Logic to initialize read side
              if (dup2(fd[2*t-2], STDIN_FILENO) == -1) {
                fprintf(stderr, "dup2() fialed!\n");
                exit(EXIT_FAILURE);
              }
              //Logic to initialize write side
              if (dup2(fd[2*t+1], STDOUT_FILENO) == -1) {
                fprintf(stderr, "dup2() fialed!\n");
                exit(EXIT_FAILURE);
              }

              close(fd[2*t-2]);
              close(fd[2*t+1]);
              }

            execvp(P->tasks[t].cmd, P->tasks[t].argv);
            printf("Error Failed to exec for %s command \n", P->tasks[t].cmd);
            exit(EXIT_FAILURE);
                
          //Parent Case
          default:

            //First Task
            if ((t==0) && (P->ntasks > 1)){ 
            close(fd[2*t+1]);
            }

            //Intermediate Tasks
            else if (((t+1) < ntasks) && (ntasks > 1)){ 
            close(fd[2*t+1]);
            close(fd[2*t-2]);
            } 

            //Last Task
            else if (((t+1) == ntasks) && (ntasks > 1)){ 
            close(fd[2*t-2]);
            }
            break;
        }
      }
    else {
      printf ("pssh: command not found: %s\n", P->tasks[t].cmd);
      break;
    }
  } 

    //Will wait for however many times forked()
    for (t=0; t != ntasks; t++){
      wait(NULL);
      }

    if (P->infile){
      if (dup2(stdin_cpy,0) == -1){
      fprintf(stderr, "dup2() fialed!\n");
      exit(EXIT_FAILURE);
      }
      close(stdin_cpy);
    }

    if (P->outfile){
      if (dup2(stdout_cpy,1) == -1){
      fprintf(stderr, "dup2() fialed!\n");
      exit(EXIT_FAILURE);
      }
      close(stdout_cpy);
    }
    return;
}

int main (int argc, char** argv)
{
    char* cmdline;
    Parse* P;

    print_banner ();

    while (1) {
        cmdline = readline (build_prompt());
        if (!cmdline)       /* EOF (ex: ctrl-d) */
            exit (EXIT_SUCCESS);

        P = parse_cmdline (cmdline);
        if (!P)
            goto next;

        if (P->invalid_syntax) {
            printf ("pssh: invalid syntax\n");
            goto next;
        }

#if DEBUG_PARSE
        parse_debug (P);
#endif

        execute_tasks (P);

    next:
        parse_destroy (&P);
        free(cmdline);
    } 
}
