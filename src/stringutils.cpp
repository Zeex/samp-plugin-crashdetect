// Copyright (c) 2018 Zeex
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
#include "stringutils.h"

#if defined LINUX
  #include <strings.h>
#elif defined _MSC_VER
  #include <string.h>
  #define strcasecmp _stricmp
#endif

namespace stringutils {

void SplitString(const std::string &s,
                 char delim,
                 std::vector<std::string> &parts) {
  std::string::size_type begin = 0;
  std::string::size_type end;

  while (begin < s.length()) {
    end = s.find(delim, begin);
    end = (end == std::string::npos) ? s.length() : end;
    parts.push_back(std::string(s.begin() + begin, s.begin() + end));
    begin = end + 1;
  }
}

template<typename CharTransformer>
std::string TransformString(const std::string &s, 
                            CharTransformer transformer) {
  std::string t;
  t.reserve(s.length());
  for (std::string::const_iterator it = s.begin(); it != s.end(); it++) {
    t.push_back(transformer(*it));
  }
  return t;
}

std::string ToLower(const std::string &s) {
  return TransformString(s, ::tolower);
}

std::string ToUpper(const std::string &s) {
  return TransformString(s, ::toupper);
}

int CompareIgnoreCase(const char *s1, const char *s2) {
  return strcasecmp(s1, s2);
}

int CompareIgnoreCase(const std::string &s1, const std::string &s2) {
  return CompareIgnoreCase(s1.c_str(), s2.c_str());
}

} // namespace stringutils
