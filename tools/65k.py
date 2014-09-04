#!/usr/bin/env python

import sys

if len(sys.argv) > 1:
  num_lines = int(sys.argv[1])
else:
  num_lines = 2**16

print("""
#include <a_samp>

halt() {
\t#emit halt 1
}

main() {
""")

for i in range(1, num_lines + 1):
  print('\tprint("%i");' % i)

print("""
\thalt();
}
""")
