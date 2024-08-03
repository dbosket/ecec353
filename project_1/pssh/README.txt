Project 1
Demetrius Bosket
14332129

How To Compile & Run
====================
$ make
$ ./pssh
$ command_1 [< infile] [| command_n]* [> outfile] [&]

where:
  Items in brackets [ ] are optional
  Items in starred brackets [ ]* are optional but can be repeated
  Non-bracketed items are required


NOTE: If you recieve "./pssh: error while loading shared libraries: libreadline.so.6:
cannot open shared object file: No such file or directory"
$ rm *.o
$ make
$ ./pssh 
$ command_1 [< infile] [| command_n]* [> outfile] [&]


Description
===========
This program creates a primitive shell application capable of executing single or multiple piped commands, input redirection, and output redirection. The primary part of this application implemented by the student was the function 'execute_tasks()' in the 'pssh.c' file. 

The function takes in a data structure called Parse, which is comprised of one or more tasks, an input file, an output file, and a few other struct members. If there is an input file or output file, the program redirects stdin or stdout as appropriate, saving copies of the original stdin and stdout for reference later. The function then enters a loop iterating over each task and executing it as a built-in command or an on-disk command as appropriate.

If the task is a built in command, the process simply calls the builtin_execute from the "builtin.c" file. Otherwise, if the command(s) are on-disk commands, then the program executes a fork(), allocating, assigning, and closing pipes as appropriate to enable interprocess communication. After all tasks have been executed, the application reassigns the original stdin or stdout as appropriate.

The user may input the next command until they choose to exit and return to the native shell application.
