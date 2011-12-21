crashdetect plugin for SA:MP server
===================================

This plugin helps in debugging SA:MP server crashes and AMX run-time errors. Because the latter are (almost) always
ignored by the server this is currently the only way to catch them.

### Demo 1: Crash by native function ###

Imagine that you have the following script:

	#include <a_samp>

	main() {
		function1();
  	}

	function1() {
		function2();
	}

	function2() {
		new buf[10];
		fread(File:123, buf); // BOOM!
	}

If crashdetect is loaded, the output would be something like this (in server log):

	[05:57:42]: The server has been crashed by 'gamemodes\crash.amx'.
	[05:57:42]: Call stack (most recent call first):
	[05:57:42]:   File 'crash.pwn', line 13
	[05:57:42]:     native fread()
	[05:57:42]:   File 'crash.pwn', line 8
	[05:57:42]:     function2()
	[05:57:42]:   File 'crash.pwn', line 4
	[05:57:42]:     function1()
	[05:57:42]:   File 'crash.pwn'
	[05:57:42]:     main()

Aha! 


### Demo 2: Run-time error ###

Consider this code:

	public OnGameModeInit() {
		do_out_of_bounds();
	}

	stock do_out_of_bounds()
	{
		new a[5];
		new i = 100;
		printf("%d", a[i]);
 	}

What would this output? Nothing! Guess why? Right, the array index we specified is out of array bounds. 
This may be very confusing sometimes, especially if you are a beginner - the server doesn't report any error
though it doesn't execute the code which follows.

Again, with crashdetect you get something like this:

	[05:48:52]: Script[gamemodes\bounds.amx]: In file 'bounds.pwn' at line 18:
	[05:48:52]: Script[gamemodes\bounds.amx]: Run time error 4: "Array index out of bounds"
	[05:48:52]: Additional information:
	[05:48:52]:   Array max index is 4 but accessing an element at 100
	[05:48:52]: Call stack (most recent call first):
	[05:48:52]:   File 'bounds.pwn', line 7
	[05:48:52]:     do_out_of_bounds()
	[05:48:52]:   File 'bounds.pwn'
	[05:48:52]:     public OnGameModeInit()

As you can see, it even tells you the exact line where the error occurs, the value of array index, etc which 
is typically enough to fix the problem.

Usage Notes
-----------

To get as much helpful output as possible you need to tell the Pawn compiler to include debug information
into the compiled .amx by specifying either -d2 or -d3 flag on the command line (or through `pawn.cfg`).

Known Issues
------------

*	The plugin can crash your server when it attempts to load debug info from a compiled .amx
	if it's more than 2^16 lines of code (#include's are counted too). Unfortunately this can't be 
	fixed without editing Pawn compiler. Also see https://github.com/Zeex/crashdetect/issues/5.
