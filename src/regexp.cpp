// Copyright (c) 2014-2021 Zeex
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

#include <cstring>
#include "regexp.h"

RegExp::RegExp(const std::string &pattern)
  : re_(nullptr)
{
  int errornumber;
  PCRE2_SIZE erroroffset = 0;
  re_ = pcre2_compile(reinterpret_cast<PCRE2_SPTR8>(pattern.c_str()),
                      PCRE2_ZERO_TERMINATED,
                      0,
                      &errornumber,
                      &erroroffset,
                      nullptr);
}

RegExp::~RegExp() {
  pcre2_code_free(re_);
}

bool RegExp::Test(const std::string &string) const {
  if (re_ == nullptr) {
    return false;
  }
  pcre2_match_data *match_data =
      pcre2_match_data_create_from_pattern(re_, NULL);
  int result = pcre2_match(re_,
                           reinterpret_cast<PCRE2_SPTR8>(string.c_str()),
                           string.length(),
                           0,
                           0,
                           match_data,
                           nullptr);
  pcre2_match_data_free(match_data);
  return result >= 0;
}
