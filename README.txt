crashdetect - https://github.com/Zeex/samp-plugin-crashdetect
=============================================================

Frequently asked questions
--------------------------

Q: How do I make crashdetect print function names, line numbers, etc?

A: You have to tell the Pawn compiler to include debugging information in the
   output AMX file. This can be accomplished by passing either -d2 or -d3 flag
   to it at the command line (because its a console application). The easiest
   way of doing that is:

     1. create a file called "pawn.cfg" inside your "pawno" directory if it
        does not already exist
     2. open the file in your favourite text editor and append "-d3" to the 
        contents

   This file is basically a list of space-separated flags that will be passed
   to the compiler whenever you hit F5 in Pawno.

   If you use someting other than Pawno you might need to put that file in the
   same directory the script you compile lives because most editors don't set
   current working directory to "pawno". It might be even easier to add the exta
   flags via your editor's options.


Q: I did everyting you said but still don't see the extra info in my error
   messages! Why??

A: There can be a few reasons for this.

   One is that the AMX file in question is put in some custom directory other
   than "gamemodes" or "filterscripts". If that's the case, crashdetect is not
   able to find the AMX when it wants to load debug info. In order to fix this
   you have to specify the path manually via AMX_PATH environment variable - a
   semicolon-separated (or colon-separated on Linux) list of paths much like the
   PATH variable if you familiar with that. The paths are either relative or 
   absolute.

   Another issue is a bug of the SA-MP compiler screwing up part of debug info
   if a script that is being compiled consists of more than 65535 lines of pre-
   processed code (i.e. the script itself + all included files). Unfortunately,
   there's no workaround except splitting code in separate filterscripts so they
   are loaded as different AMX files.
