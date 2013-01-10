[crashdetect]( https://github.com/Zeex/samp-plugin-crashdetect)
===============================================================

This plugin will help you debug runtime errors and server crashes. When
something goes wrong you'll see a more or less detailed error message that
may contain a description of the error, the stack trace of the offending script
and additionally the system call stack in case of a server crash. This can
greatly reduce amount of time spent spotting the erroneous code and fixing it.

See the [original][forum] topic on the SA-MP Forums for more information and
examples.


Download
--------

Get latest binaries for Windows and Linux from the [**`downloads`**][downloads]
branch.


Things you might want to know
-----------------------------

**How do I get function names, line numbers, etc in stack trace?**

You have to tell the Pawn compiler to include debugging information in the
resulting AMX file. This can be accomplished by passing either `-d2` or `-d3`
flag to it at the command line. The easiest way is:

* Create a file called `pawn.cfg` inside your `pawno` directory if it
  does not already exist

* Open it in your favourite text editor and insert `-d3` (preceding with a space
  it the file was not empty)

This file is basically a list of flags that will be passed to the compiler
whenever you hit F5 in Pawno. If you use someting other than Pawno you might
need to put that file in the same directory as the script you compile lives
in because most editors don't set current working directory to `pawno`.
It might be easier to add the exta flags via your editor's options.


**Still doesn't work! Why??**

If you put the AMX file in some custom directory other than `gamemodes` or
`filterscripts` or in ia subdirectory of these crashdetect will not be able to
find it when loading debug info. In order to fix this you have to specify the
path manually via the `AMX_PATH` environment variable which is a
semicolon-separated (or colon-separated on Linux) list of paths, similar to
the `PATH` variable. The path can be absolute or relative to the server root.


**Execute custom command on runtime error**

It is possible to execute a custom shell command when a runtime error occurs
by setting the `run_on_error` option in `server.cfg`, e.g.

    run_on_error echo ERROR!!


**Shut down after first error**

You might want to make the server automatically exit on first runtime error:

    die_on_error 1

[forum]: http://forum.sa-mp.com/showthread.php?t=262796
[downloads]: https://github.com/Zeex/samp-plugin-crashdetect/tree/downloads
