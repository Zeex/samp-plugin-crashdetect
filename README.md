crashdetect
===========

This plugin helps in debugging of SA-MP server crashes and runtime AMX errors.

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

