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

#include <configreader.h>
#include "options.h"
#include "regexp.h"

namespace {

unsigned int TraceFlagsFromString(const std::string &s) {
  unsigned int flags = 0;
  for (std::size_t i = 0; i < s.length(); i++) {
    switch (s[i]) {
      case 'n':
        flags |= TRACE_NATIVES;
      case 'p':
        flags |= TRACE_PUBLICS;
      case 'f':
        flags |= TRACE_FUNCTIONS;
    }
  }
  return flags;
}

} // namespace

Options::Options():
  trace_flags_(0),
  trace_filter_(nullptr)
{
  ConfigReader server_cfg("server.cfg");

  trace_flags_ = TraceFlagsFromString(server_cfg.GetValueWithDefault("trace"));
  std::string trace_filter_pattern =
    server_cfg.GetValueWithDefault("trace_filter");
  if (!trace_filter_pattern.empty()) {
    trace_filter_ = new RegExp(trace_filter_pattern);
  }

  log_path_ = server_cfg.GetValueWithDefault("crashdetect_log");
  log_time_format_ =
    server_cfg.GetValueWithDefault("logtimeformat", "[%H:%M:%S]");

  long_call_time_ = server_cfg.GetValueWithDefault("long_call_time", 5000U);
}

Options::~Options() {
  delete trace_filter_;
}

// static
Options &Options::shared() {
  static Options instance;
  return instance;
}
