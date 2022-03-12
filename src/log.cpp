// Copyright (c) 2016-2021 Zeex
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
#include "log.h"
#include "logprintf.h"
#include "options.h"

namespace {

class Log {
 public:
  Log(): file_(nullptr) {
    const std::string &filename = Options::shared().log_path();
    if (!filename.empty()) {
      file_ = std::fopen(filename.c_str(), "a");
      std::setbuf(file_, nullptr);
    }
    if (file_ != nullptr) {
      time_format_ = Options::shared().log_time_format();
    }
  }

  Log(const Log &) = delete;
  Log &operator=(const Log &) = delete;

  ~Log() {
    if (file_ != nullptr) {
      std::fclose(file_);
    }
  }

  void PrintV(const char *prefix, const char *format, std::va_list va) {
    if (file_ != nullptr) {
      std::string new_format;
      if (!time_format_.empty()) {
        char time_buffer[64];
        std::time_t time = std::time(nullptr);
        std::strftime(time_buffer,
                      sizeof(time_buffer),
                      time_format_.c_str(),
                      std::localtime(&time));
        new_format.append(time_buffer);
        new_format.append(" ");
      }
      new_format.append(format);
      new_format.append("\n");
      vfprintf(file_, new_format.c_str(), va);
    } else {
      std::string new_format(prefix);
      new_format.append(format);
      vlogprintf(new_format.c_str(), va);
    }
  }

 private:
  std::FILE *file_;
  std::string time_format_;
};

} // namespace

void LogPrintV(const char *prefix, const char *format, std::va_list va) {
  static Log global_log;
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
