crashdetect
===========

This plugin helps in debugging of SA-MP server crashes and runtime AMX errors.

Notes on usage
--------------

crashdetect needs debugging information to display line numbers, file names, etc in AMX stack backtrace,
so if you want those compile your script in debug mode. To do that pass `-d3` option to the Pawn compiler,
either at the command line or through `pawn.cfg`.

Settings
--------

You can configure crashdetect by changing the following settings in server.cfg:

*	`die_on_error <0|1>` - Shut down the server on runtime error. 

	By default this option is turned off.

*	`run_on_error <command>` - Run a shell command on runtime error. 

	This can be useful if you want to perform some special action upon the error such
	as run a shell script that would notify admins, etc. 

	The command is executed right after all the error messages are printed to server log
	so you are able to read just the last few lines to copy backtrace and error text 
	instead of grepping the whole log.

How to compile
--------------

To build crashdetect from source you will need CMake 2.8.6 and newer: http://www.cmake.org/cmake/resources/software.html

### Getting the source code ###

You have two options:

*	Download a tarball or zip from the **Downloads** section:

	https://github.com/Zeex/samp-crashdetect-plugin/downloads

*	Clone Git repository:

		git clone git://github.com/Zeex/samp-crashdetect-plugin.git crashdetect

### Building the plugin ###

#### Linux ####

Run these commands in terminal:

	cd path/to/crashdetect/root
	mkdir build
	cd build
	cmake ../
	make

#### Windows ####

*	Ater installing CMake open CMake GUI frontend:

	*Start Menu -> All Programs -> CMake 2.8 -> CMake (cmake-gui)*

*	Set path to source and output directories, for example:

	*	Where is the source code: `C:\Users\Zeex\samp-server\crashdetect`
	*	Where to build binaries: `C:\Users\Zeex\samp-server\crashdetect\build`

*	Hit *Configure*. CMake will ask you to choose a generator, select your favourite tool there (leaving "Use native compilers" as is).

*	And finally, press the *Generate* button.

OK! Now use the tool that you've specified to open/compile generated project(s).
