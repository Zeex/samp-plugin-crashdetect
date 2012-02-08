crashdetect plugin
==================

This plugin helps in debugging of SA-MP server crashes and runtime AMX errors.

Usage Notes
-----------

To get source file names, line numbers, etc in backtrace compile your script with full 
debug information by using either `-d2` or `-d3` flag of pawncc.

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

*	CMake 2.8.6+ (Windows and Linux)

	http://www.cmake.org/cmake/resources/software.html
	
	or GNU make (Linux only)
