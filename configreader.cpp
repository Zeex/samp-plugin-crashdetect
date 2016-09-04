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

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#include "configreader.h"

namespace {

struct is_space : public std::unary_function<char, bool> {
  bool operator()(char c) const {
    return std::isspace(c) != 0;
  }
};

inline std::string &TrimStringLeft(std::string &s) {
  s.erase(s.begin(),
          std::find_if(s.begin(), s.end(), std::not1(is_space())));
  return s;
}

inline std::string &TrimStringRight(std::string &s) {
  s.erase(std::find_if(s.rbegin(),  s.rend(), std::not1(is_space())).base(),
          s.end());
  return s;
}

inline std::string &TrimString(std::string &s) {
  return TrimStringLeft(TrimStringRight(s));
}

} // anonymous namespace

ConfigReader::ConfigReader() {
}

ConfigReader::ConfigReader(const std::string &filename) {
  ReadFromFile(filename);
}

ConfigReader::ConfigReader(std::istream &stream) {
  ReadFromStream(stream);
}

bool ConfigReader::ReadFromFile(const std::string &filename) {
  std::ifstream stream(filename.c_str());

  if (!stream.is_open()) {
    return false;
  }

  ReadFromStream(stream);
  return true;
}

void ConfigReader::ReadFromString(const std::string &config) {
  std::istringstream stream(config);
  ReadFromStream(stream);
}

void ConfigReader::ReadFromStream(std::istream &stream) {
  std::string line;

  while (std::getline(stream, line)) {
    TrimString(line);

    std::string::iterator delimIter =
      std::find_if(line.begin(), line.end(), is_space());
    if (delimIter == line.end()) {
      continue;
    }

    std::string::iterator valueIter =
      std::find_if(delimIter, line.end(), std::not1(is_space()));
    if (valueIter == line.end()) {
      continue;
    }

    std::string name = std::string(line.begin(), delimIter);
    std::string value = std::string(valueIter, line.end());

    options_.insert(std::make_pair(name, value));
  }
}

void ConfigReader::GetValue(const std::string &name,
                            std::string &value) const {
  value = GetValueWithDefault(name, value);
}

std::string ConfigReader::GetValueWithDefault(
  const std::string &name,
  const std::string &defaultValue) const
{
  option_map::const_iterator iterator = options_.find(name);
  if (iterator != options_.end()) {
    return iterator->second;
  }
  return defaultValue;
}
