// Copyright (c) 2018-2021 Zeex
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>

class RegExp;

enum TraceFlags {
  TRACE_NONE = 0x00,
  TRACE_NATIVES = 0x01,
  TRACE_PUBLICS = 0x02,
  TRACE_FUNCTIONS = 0x04
};

class Options {
 public:
  unsigned int trace_flags()
    const { return trace_flags_; }
  unsigned int long_call_time()
    const { return long_call_time_; }
  const RegExp *trace_filter()
    const { return trace_filter_; }
  const std::string &log_path()
    const { return log_path_; }
  const std::string &log_time_format()
    const { return log_time_format_; }

  static Options &shared();

 private:
  Options(const Options &options);
  Options();
  Options &operator=(const Options &options) = delete;
  ~Options();

 private:
  unsigned int trace_flags_;
  unsigned int long_call_time_;
  RegExp *trace_filter_;
  std::string log_path_;
  std::string log_time_format_;
};

#endif // !OPTIONS_H
