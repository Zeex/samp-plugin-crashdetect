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

#include <ctime>
#include <cstdio>

#include <configreader.h>

#include "log.h"
#include "logprintf.h"

namespace {

class Log {
 public:
  Log(): file_(0) {
    ConfigReader server_cfg("server.cfg");
    std::string filename = server_cfg.GetValueWithDefault("crashdetect_log");
    if (!filename.empty()) {
      file_ = std::fopen(filename.c_str(), "a");
      std::setbuf(file_, 0);
    }
    if (file_ != 0) {
      time_format_ =
        server_cfg.GetValueWithDefault("logtimeformat", "[%H:%M:%S]");
    }
  }

  ~Log() {
    if (file_ != 0) {
      std::fclose(file_);
    }
  }

  void PrintV(const char *prefix,
              const char *format,
              std::va_list va) {
    std::string new_format;

    if (file_ != 0) {
      if (!time_format_.empty()) {
        char time_buffer[128];
        std::time_t time = std::time(0);
        std::strftime(time_buffer,
                      sizeof(time_buffer),
                      time_format_.c_str(),
                      std::localtime(&time));
        new_format.append(time_buffer);
        new_format.append(" ");
      }
    } else {
      new_format.append(prefix);
    }

    new_format.append(format);

    if (file_ != 0) {
      new_format.append("\n");
      vfprintf(file_, new_format.c_str(), va);
    } else {
      vlogprintf(new_format.c_str(), va);
    }
  }

 private:
  std::FILE *file_;
  std::string time_format_;
};

Log global_log;

} // namespace

void LogPrintV(const char *prefix, const char *format, std::va_list va) {
  global_log.PrintV(prefix, format, va);
}

void LogTracePrint(const char *format, ...) {
  std::va_list va;
  va_start(va, format);
  LogPrintV("[trace] ", format, va);
  va_end(va);
}

void LogDebugPrint(const char *format, ...) {
  std::va_list va;
  va_start(va, format);
  LogPrintV("[debug] ", format, va);
  va_end(va);
}
