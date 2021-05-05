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

#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#ifndef CONFIGREADER_H
#define CONFIGREADER_H

class ConfigReader {
 public:
  typedef std::map<std::string, std::string> option_map;

  ConfigReader();
  ConfigReader(const std::string &filename);
  ConfigReader(std::istream &stream);

  bool ReadFromFile(const std::string &filename);
  void ReadFromString(const std::string &config);
  void ReadFromStream(std::istream &stream);

  void GetValue(const std::string &name, std::string &value) const;

  template<typename T>
  void GetValue(const std::string &name, T &value) const;

  template<typename T>
  T GetValueWithDefault(const std::string &name,
                        const T &defaultValue = T()) const;

  std::string GetValueWithDefault(
    const std::string &name,
    const std::string &defaultValue = std::string()) const;

  template<typename T>
  void GetValues(const std::string &name, std::vector<T> &values) const;

  template<typename T>
  std::vector<T> GetValues(const std::string &name) const;

 private:
  option_map options_;
};

template<typename T>
void ConfigReader::GetValue(const std::string &name, T &value) const {
  value = GetValueWithDefault(name, value);
}

template<typename T>
T ConfigReader::GetValueWithDefault(const std::string &name,
                                    const T &defaultValue) const {
  option_map::const_iterator iterator = options_.find(name);
  if (iterator == options_.end()) {
    return defaultValue;
  }

  std::istringstream sstream(iterator->second);
  T value;
  sstream >> value;

  if (sstream) {
    return value;
  }

  return defaultValue;
}

template<typename T>
void ConfigReader::GetValues(const std::string &name,
                             std::vector<T> &values) const {
  values = GetValuesWithDefault(name, values);
}

template<typename T>
std::vector<T> ConfigReader::GetValues(const std::string &name) const {
  option_map::const_iterator iterator = options_.find(name);
  if (iterator == options_.end()) {
    return std::vector<T>();
  }

  std::vector<T> values;
  std::istringstream sstream(iterator->second);
  std::copy(std::istream_iterator<T>(sstream),
            std::istream_iterator<T>(),
            std::back_inserter(values));

  if (sstream || values.size() > 0) {
    return values;
  }

  return std::vector<T>();
}

#endif // !CONFIGREADER_H
