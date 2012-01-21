crashdetect plugin for SA:MP server
===================================

This small plugin helps in debugging SA:MP server crashes and AMX run-time errors. Because the latter are (almost) always
ignored by the server this is currently the only way to catch them.

Basically it just prints a call stack when an error occurs and sometimes can provide some extra info. See the two
demo cases below.

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

	[05:31:41] [debug] The server has crashed executing 'crash.amx'
	[05:31:41] [debug] [crash.amx]: Call Stack (most recent call first):
	[05:31:41] [debug] [crash.amx]: #0 native fread() from samp-server.exe
	[05:31:41] [debug] [crash.amx]: #1 function2() at crash.pwn:13
	[05:31:41] [debug] [crash.amx]: #2 function1() at crash.pwn:8
	[05:31:41] [debug] [crash.amx]: #3 main() at crash.pwn:4

so now you know what's wrong.


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

What would this output? Nothing! Guess why? Right! The array index we specified is out of array bounds:

	[05:33:03] [debug] [bounds.amx]: Run time error 4: "Array index out of bounds"
	[05:33:03] [debug] [bounds.amx]: Accessing element at index 100 past array upper bound 4
	[05:33:03] [debug] [bounds.amx]: Call Stack (most recent call first):
	[05:33:03] [debug] [bounds.amx]: #0 do_out_of_bounds() at bounds.pwn:14
	[05:33:03] [debug] [bounds.amx]: #1 public OnGameModeInit() at bounds.pwn:7

As you can see, it even tells you the error line, array index, etc which is enough to fix the problem.

Usage Notes
-----------

To get as much helpful output as possible (e.g. line numbers) you need to tell the Pawn compiler to 
produce debugging information by passing either -d2 or -d3 flag on the command line or through *pawn.cfg*.

Plugin Options
--------------

You can configure crashdetect by changing the following settings in server.cfg:

*	`die_on_error <0|1>` - Shut down the server on runtime or native error. 
	By default this option is turned off.

Build Requirements
------------------

To build crashdetect from source you need:

*	Boost Libraries 1.47+

	http://www.boost.org/users/download/

*	CMake 2.8.6+

	http://www.cmake.org/cmake/resources/software.html

Known Issues
------------

*	The plugin can crash your server when it attempts to load debug info from a script that is
	more than 2^16 lines long (together with #include's). This is due to a compiler bug/limit.
