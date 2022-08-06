# SHell-mush-
by: Kyle Kesler

mush has nowhere near the functionality of a full-blown shell like /bin/sh or
/bin/csh, but it is fairly powerful for its size and has most features that one
would want in a shell, including:

Both interactive and batch processing - If mush is run with no argument it reads
    commands from stdin until an end of file (^D) is typed. If mush is run with an
    argument, e.g. mush foofile it will read its commands from foofile.

Support for redirection - mush supports redirection of standard in (<) and
    standard out (>) from or into files. mush also supports pipes (|) to connect
    the standard output of one command to the standard input of the following one.

A built-in cd command.

Execution of various unix command line programs.

Support for SIGINT. When the interrupt character (^C) is typed, mush catches
    the resulting SIGINT and responds appropriately. That is, the shell doesnt die,
    but it waits for any running children to terminate and resets itself to a
    sane state.
    
Maximum macros implemented:
- the maximum length of the command line: 512 bytes
- the maximum length of the pipeline: 20
- the maximum number of arguments to any one command: 20
