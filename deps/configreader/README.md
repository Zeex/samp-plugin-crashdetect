# Config Reader

[![Build Status][build_status]][build]
[![Build Status - Windows][build_status_win]][build_win]

A small library to read space-separated key-value configuration files like this:

```
int_var 1
float_var 1.5
string_var some string here
vector_var 1 2 3 4 5
```

Building
--------

You can CMake to build this library from source:

```bash
cd configreader
mkdir build && cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release
make
```

Or you can simply drop `configreader.h` and `configreader.cpp` in your project.

Usage
-----

See [this file][example] for some examples of using ConfigReader.

License
-------

```
Copyright (c) 2016 Zeex
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
```

[example]: https://github.com/Zeex/configreader/blob/master/configreader_tests.cpp
[build]: https://travis-ci.org/Zeex/configreader
[build_status]: https://travis-ci.org/Zeex/configreader.svg?branch=master
[build_win]: https://ci.appveyor.com/project/Zeex/configreader/branch/master
[build_status_win]: https://ci.appveyor.com/api/projects/status/4msd2wh1qihitiy4?svg=true
