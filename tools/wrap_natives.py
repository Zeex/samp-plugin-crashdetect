#!/usr/bin/env python
#
# Copyright (c) 2014-2015 Zeex
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import re
import sys

def main(argv):
  natives = []
  for filename in argv[1:]:
    with open(filename, 'r') as f:
      for line in f.readlines():
        match = re.match(r'native\s+([a-zA-Z_@][a-zA-Z0-9_@]*\(.*?\))\s*;',
                         line, re.MULTILINE)
        if match is not None:
          native = match.group(1)
          natives.append(native)
  for native in natives:
    name = re.sub(r'(.*)\(.*\)', r'\1', native)
    params = re.sub(r'.*\((.*)\)', r'\1', native)
    param_names = [m.group(1)
      for m in re.finditer(
        r'(?:const\s+)?'
        r'(?:(?:{.*?}|\S+):\s*)?'
        r'(\S+?)'
        r'(?:\s*\[.*?\])?'
        r'(?:\s*=\s*.+)?'
        r'\s*(?:,|$)', params)]
    if '...' not in param_names:
      print('stock _%s(%s) {' % (name, params))
      print('\treturn %s(%s);' % (name, ', '.join(param_names)))
      print('}')
      print('#define %s _%s\n' % (name, name))

if __name__ == '__main__':
  sys.exit(main(sys.argv))
