crashdetect plugin
==================

This plugin helps in debugging SA:MP server crashes and runtime AMX errors.

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

To build crashdetect from source you will need:

*	Boost 1.47+

	http://www.boost.org/users/download/

*	CMake 2.8.6+

	http://www.cmake.org/cmake/resources/software.html
