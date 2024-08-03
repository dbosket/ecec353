#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "builtin.h"
#include "parse.h"


static char* builtin[] = {
    "exit",   /* exits the shell */
    "which",  /* displays full path to command */
    NULL
};


int is_builtin (char* cmd)
{
    int i;

    for (i=0; builtin[i]; i++) {
        if (!strcmp (cmd, builtin[i]))
            return 1;
    }

    return 0;
}


void builtin_execute (Task T)
{

    //Logic for exit command
    if (!strcmp (T.cmd, "exit")) {
        exit (EXIT_SUCCESS);
    }

    //Which should follow command_found and return the directory it made on
    //line 76-83
    //Logic for which command
    else if (!strcmp (T.cmd, "which")) {
      switch(fork()) {
        case -1:
          fprintf(stderr, "ERROR: failed to fork() for %s\n command\n", T.cmd);
          exit(EXIT_FAILURE);

        //Child Case
        case 0:
          
          //if given as second or third arg, this doesn't work
          if (is_builtin(T.argv[1])){
           printf("%s: shell built-in command\n", T.argv[1]); 
           exit(EXIT_SUCCESS);
          }
          
          else{
            execvp(T.cmd, T.argv);
            fprintf(stderr,"Error Failed to exec for %s\n command\n", T.cmd);
            exit(EXIT_FAILURE);
          }
          
        //Parent Case
        default:
          wait(NULL);
          break;
      }
      return;

    }
 
    else {
        printf ("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
}
