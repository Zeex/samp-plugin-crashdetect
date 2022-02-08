[CrashDetect plugin][github]
============================

[![Version][version_badge]][version]
[![Build Status][build_status]][build]

CrashDetect helps you debug runtime errors and server crashes. When something
goes wrong you will get a detailed error message with error description, stack
trace, and other useful information that will make it easier to quickly find
and fix the issue.

Installation
------------

1. Download a binary package from the [Releases][download] page on Github or
   build it yourself from source code (see
   [Building from source code](#building-from-source-code)).
2. Extract and copy `crashdetect.so` or `crashdetect.dll` to `<sever>/plugins/`.
3. Add `crashdetect` (Windows) or `crashdetect.so` (Linux) to the `plugins`
   line of your server.cfg.

Binary packages come with an include file (`crashdetect.inc`) that contains
some helper functions that you may find useful. But **you don't have to
include** it to be able to use CrashDetect.

Usage
-----

Apart from installing the plugin you don't have to do anything further to
start receiving errors reports. By default all errors will be saved in your
`server_log.txt`, but this can be changed
(see [Configuration](#configuration)).

For better debugging experience, make sure that you
[compile your script with debug info enabled][debug_info]. Doing this will let
you see more information in stack traces such as function names, parameter names
and values, source file names and line numbers.

Please be aware that when using this plugin your code WILL run slower due
to the overhead associated with detecting errors and providing accurate
error information (for example, some runtime optimizations are disabled).
Usually this is fine during development, but it's not recommended to load
CrashDetect on a production (live) server with many players.

Configuration
-------------

CrashDetect reads settings from server.cfg, the server configuration file. This
is done during plugin loading, so if you change any settings you will probably
need to restart your server.

Available settings:

* `trace <flags>`

  Enables function call tracing.

  If enabled, CrashDetect will show information about every function call in
  all running scripts, such as the name of the function being called and the
  values of its parameters.

  `flags` may be one or combination of the following:

  * `n` - trace native functions
  * `p` - trace public functions
  * `f` - trace normal functions (i.e. all non-public functions)

  For example, `trace pn` will trace both public and native calls, and
  `trace pfn` will trace all functions.

* `trace_filter <regexp>`

  Filters `trace` output based on a regular expression.

  Examples:

  * `trace_filter Player` - output functions whose name contains `Player`
  * `trace_filter playerid=0` - show functions whose `playerid` parameter is 0

* `crashdetect_log <filename>`

  Use a custom log file for output.

  By default all diagnostic information is printed to the server log. This
  option lets you redirect output to a separate file.

* `long_call_time <us>`

  How long a top-level callback call should last before crashdetect prints a
  warning.  This can be set very high (for example `1000000`) to only detect
  functions that have totally hung, or very low (`500`) to detect functions that
  complete, but are just slow (thus affecting overall server execution and
  sync).  Default value is `5000` (5 milliseconds).

  Use `0` to disable this check.

Address Naught
--------------

Often writing to address naught is a mistake, it indicates that some target address is unset,
especially in larger modes.  `address_naught` mode detects all writes to this address and gives a
new (to VM) error in that case - `AMX_ERR_ADDRESS_0`.  Note that this isn't always an error, it is a
valid address and real data can be stored there, so if this detection is enabled the mode must
ensure that nothing important will be written there (fixes.inc does this by defining and not using
the anonymous automata).  This is also the reason why there is no global config option for this
detection - it is by necessity done on a per-mode basis, is off by default, and can only be enabled
by functions (technically by registers).

Functions
---------

There are several functions defined in `crashdetect.inc` to access plugin information.  These are
all just wrappers around direct register accesses, but provide a much nicer API.

* `bool:IsCrashDetectPresent();` - Is the crashdetect plugin loaded?
* `SetCrashDetectLongCallTime(us_time);` - Set the long call warning threshold.
* `GetCrashDetectLongCallTime();` - Get the long call warning threshold.
* `DisableCrashDetectLongCall();` - Disable the long call warning.
* `EnableCrashDetectLongCall();` - Disable the long call warning.
* `ResetCrashDetectLongCallTime();` - Reset the long call threshold to the default (from `server.cfg`).
* `RestartCrashDetectLongCall();` - Restart the long call timer.
* `bool:IsCrashDetectLongCallEnabled();` - Is long function call detection enabled?
* `bool:HasCrashDetectLongCall();` - Does the current version of crashdetect support this feature?
* `GetCrashDetectDefaultTime();` - Get the default long call time threshold.
* `DisableCrashDetectAddr0();` - Disable address naught write detection in this mode.
* `EnableCrashDetectAddr0();` - Enable address naught write detection in this mode.
* `bool:IsCrashDetectAddr0Enabled();` - Is the error currently enabled?
* `bool:HasCrashDetectAddr0();` - Does the current version of crashdetect support this feature?

Registers
---------

The plugin adds two control registers accessed via `LCTRL` and `SCTRL` - `0xFF` for general flags
and `0xFE` for long call time values:

```pawn
// Get the current long call time (even when it is disabled).
#emit ZERO.pri          // Always set pri to 0 before `LCTRL` calls.
#emit LCTRL        0xFE // Will set `pri` if the plugin is loaded.
#emit STOR.pri     var  // Save the result.
```

```pawn
// Enable address naught write detection.
#emit CONST.pri    192  // 64 (address_naught control bit) | 128 (address_naught enable).
#emit SCTRL        0xFF // Set the register.
```

The flags are:

* `1` - Cashdetect present (read only).
* `2` - long_call_time checks enabled (write ignored when `server.cfg` has `long_call_time 0`).
* `4` - long_call_time reset to default time (write `1` only).
* `8` - long_call_time restart check from now (write `1` only).
* `16` - Error with the crashdetect user data.
* `32` - long_call_time control bit.
* `64` - address_naught control bit.
* `128` - address_naught detection enabled.

When read-only values are set, they are ignored.  When write-only bits are returned they are always
`0`.  When write-1-only bits are `0` they are ignored, the `1` is a signal to trigger something.  To
set most registers the relevant control bit must also be set.  So to enable address naught detection
requires bit `6` (`64`) and bit `7` (`128`).  To disable it just requires bit `6`.  Note that while
many of these bits would seem to be independent you cannot disable address naught detection, enable
long call detection, and reset and restart the timer all at once with `0x6E`; only one command at
once will work.

Building from source code
-------------------------

If you want to build CrashDetect from source code, e.g. to fix a bug and
submit a pull request, simply follow the steps below. You will need a C++
compiler and CMake.

### Linux

Install gcc and g++, make and cmake. On Ubuntu you would do that like so:

```
sudo apt-get install gcc g++ make cmake
```

If you're on a 64-bit system you'll need additional packages for compiling
for 32-bit:

```
sudo apt-get install gcc-multilib g++-multilib
```

For CentOS:

```
yum install gcc gcc-c++ cmake28 make
```

Now you're ready to build CrashDetect:

```
cd crashdetect
mkdir build && cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
make
```

### Windows

You'll need to install CMake and Visual Studio (Express edition will suffice).
After that, either run cmake from the command line:

```
cd crashdetect
mkdir build && cd build
path/to/cmake.exe ../ -DBUILD_TESTING=OFF
```

or do the same from cmake-gui. This will generate a Visual Studio project in
the build folder.

To build the project:

```
path/to/cmake.exe --build . --config Release
```

You can also build it from within Visual Studio: open build/crashdetect.sln
and go to menu -> Build -> Build Solution (or just press F7).

License
-------

Licensed under the 2-clause BSD license. See [LICENSE.txt](LICENSE.txt).

[github]: https://github.com/Zeex/samp-plugin-crashdetect
[version]: http://badge.fury.io/gh/Zeex%2Fsamp-plugin-crashdetect
[version_badge]: https://badge.fury.io/gh/Zeex%2Fsamp-plugin-crashdetect.svg
[build]: https://ci.appveyor.com/project/Zeex/samp-plugin-crashdetect/branch/master
[build_status]: https://ci.appveyor.com/api/projects/status/nay4h3t5cu6469ic/branch/master?svg=true
[download]: https://github.com/Zeex/samp-plugin-crashdetect/releases
[debug_info]: https://github.com/Zeex/samp-plugin-crashdetect/wiki/Compiling-scripts-with-debug-info
