[CrashDetect][github] 
=====================

[![Donate][donate_button]][donate]

This plugin helps you debug runtime errors and server crashes. When something
goes wrong you see a more or less detailed error message that contains a
description of the error and a stack trace. 

See the [original][forum] topic on the SA-MP Forums for more information and
examples.

Download
--------

Get latest binaries for Windows and Linux [here][download].

FAQ
---

### How do I get function names, line numbers, etc in stack trace?

You have to tell the Pawn compiler to include debugging information in the
output AMX file. To do this pass the `-d3` flag to pawncc at the command line.
Alternatively, you can create a file called `pawn.cfg` in your `pawno` folder
and put your command line options in there separating them with spaces.

Note that if you put the AMX file in some custom directory other than
`gamemodes` and `filterscripts` or in a subdirectory of these CrashDetect will
not find the file when loading debug info. In order to fix this you can set
the `AMX_PATH` environment variable to point to the right folder or a list of
folders. Its format is similar to that of `PATH`. The paths can be either
absolute or relative to the server root.

### Is it possible to perform some action whenever a runtime error occurs?

Yes, use the OnRuntimeError callback. Its signature is as follows:

```c++
public OnRuntimeError(error_code, &bool:suppress);
```

Setting `suppress` to `true` will suppress the error message.

[github]: https://github.com/Zeex/samp-plugin-crashdetect
[forum]: http://forum.sa-mp.com/showthread.php?t=262796
[download]: https://github.com/Zeex/samp-plugin-crashdetect/releases 
[donate]: http://pledgie.com/campaigns/19750
[donate_button]: http://www.pledgie.com/campaigns/19750.png
