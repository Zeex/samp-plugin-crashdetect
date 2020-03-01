[CrashDetect plugin][github]
============================

[![Version][version_badge]][version]
[![Build Status][build_status]][build]
[![Build Status - Windows][build_status_win]][build_win]

CrashDetect helps you debug runtime errors and server crashes. When something
goes wrong you will get a detailed error message with error description, stack
trace, and other useful information that will makes it easier to quickly find
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
[compile your script with debug info enabled][debug-info]. Doing this will let
you see more information in stack traces such as function names, prameter names
and values, source file names and line numbers.

Please be aware that when using this plugin your code WILL run slower due
to the overhead associated with detecting errors and providing accurate
error information (for example, some runtime optimizations are diasbled).
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
[build]: https://travis-ci.org/Zeex/samp-plugin-crashdetect
[build_status]: https://travis-ci.org/Zeex/samp-plugin-crashdetect.svg?branch=master
[build_win]: https://ci.appveyor.com/project/Zeex/samp-plugin-crashdetect/branch/master
[build_status_win]: https://ci.appveyor.com/api/projects/status/nay4h3t5cu6469ic/branch/master?svg=true
[download]: https://github.com/Zeex/samp-plugin-crashdetect/releases
[debug-info]: https://github.com/Zeex/samp-plugin-crashdetect/wiki/Compiling-scripts-with-debug-info
