[CrashDetect plugin][github]
============================

[![Version][version_badge]][version]
[![Build Status][build_status]][build]
[![Build Status - Windows][build_status_win]][build_win]

This plugin helps you debug runtime errors and server crashes. When something
goes wrong you see a more or less detailed error message that contains a
description of the error and a stack trace.

Installing
----------

1. Download a compiled plugin form the [Releases][download] page on Github or
build it yourself from source code (see below).
2. Extract/copy `crashdetect.so` or `crashdetect.dll` to `<sever>/plugins/`.
3. Add `crashdetect` (Windows) or `crashdetect.so` (Linux) to the `plugins` line of your server.cfg.

Building on Linux
-----------------

Install gcc and g++, make and cmake. On Ubuntu you would do that with:

```
sudo apt-get install gcc g++ make cmake
```

If you're building on a 64-bit system you'll need multilib packages for gcc and g++:

```
sudo apt-get install gcc-multilib g++-multilib
```

If you're building on CentOS, install the following packages:

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

Building on Windows
-------------------

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

You can also build it from within Visual Studio: open build/crashdetect.sln and
go to menu -> Build -> Build Solution (or just press F7).

Configuration
-------------

CrashDetect reads settings from server.cfg, the server configuration file.
Below is the list of available settings along with some examples.

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

  * `trace_filter Player`     - output functions whose name contains `Player`
  * `trace_filter playerid=0` - show functions whose `playerid` parameter is 0

* `crashdetect_log <filename>`
  
  Use a custom log file for output.

  By default crashes and errors are printed to the server log. This options
  allows you to set a different file for that.

Debug info
----------

To get as much useful information as possible in crash and runtime error reports
during debugging [compile your script(s) with debug info][debug-info].

License
-------

Licensed under the 2-clause BSD license. See the LICENSE.txt file.

[github]: https://github.com/Zeex/samp-plugin-crashdetect
[version]: http://badge.fury.io/gh/Zeex%2Fsamp-plugin-crashdetect
[version_badge]: https://badge.fury.io/gh/Zeex%2Fsamp-plugin-crashdetect.svg
[build]: https://travis-ci.org/Zeex/samp-plugin-crashdetect
[build_status]: https://travis-ci.org/Zeex/samp-plugin-crashdetect.svg?branch=master
[build_win]: https://ci.appveyor.com/project/Zeex/samp-plugin-crashdetect/branch/master
[build_status_win]: https://ci.appveyor.com/api/projects/status/nay4h3t5cu6469ic/branch/master?svg=true
[download]: https://github.com/Zeex/samp-plugin-crashdetect/releases
[debug-info]: https://github.com/Zeex/samp-plugin-crashdetect/wiki/Compiling-scripts-with-debug-info
