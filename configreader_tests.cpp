// Copyright (c) 2016 Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "configreader.h"

#define CHECK_EQUAL(x, y) \
  Check((x) == (y), #x, x, #y, y, '=')

void OK() {
  std::cout << "OK" << std::endl;
  std::exit(EXIT_SUCCESS);
}

void Failed() {
  std::cout << "Failed" << std::endl;
  std::exit(EXIT_FAILURE);
}

template<typename T, typename U>
void Check(bool result,
           const char *op1,
           const T &op1_value,
           const char *op2,
           const U &op2_value,
           char rel) {
  if (!result) {
    std::cout << "Check failed: "
              << op1 << " `" << op1_value << "`"
              << " " << rel << " "
              << op2 << " `" << op2_value << "`"
              << std::endl;
    Failed();
  }
}

int main() {
  ConfigReader config;
  config.ReadFromString(
    "i 1\n"
    "f 1.5\n"
    "s string with\twhitespace\n"
    "v \thello \tworld\n"
  );

  int i = config.GetValueWithDefault("i", 0);
  CHECK_EQUAL(i, 1);

  double f = config.GetValueWithDefault("f", 0.0);
  CHECK_EQUAL(f, 1.5);

  std::string s = config.GetValueWithDefault("s", "");
  CHECK_EQUAL(s, "string with\twhitespace");

  std::vector<std::string> v = config.GetValues<std::string>("v");
  CHECK_EQUAL(v.size(), 2);
  CHECK_EQUAL(v[0], "hello");
  CHECK_EQUAL(v[1], "world");

  OK();
}
