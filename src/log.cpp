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
#include <cstring>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include "log.h"
#include "logprintf.h"
#include "options.h"

namespace {

const size_t BUFFER_SIZE = 8192; // 8 kb buffer

class Log {
 public:
  Log() : file_(nullptr), buffer_(new char[BUFFER_SIZE]), stop_thread_(false) {
    const std::string &filename = Options::shared().log_path();
    if (!filename.empty()) {
      file_ = std::fopen(filename.c_str(), "a");
      if (file_ != nullptr) {
        std::setvbuf(file_, buffer_.get(), _IOFBF, BUFFER_SIZE);
      }
    }
    if (file_ != nullptr) {
      time_format_ = Options::shared().log_time_format();
    }
    log_thread_ = std::thread(&Log::ProcessQueue, this);
  }

  Log(const Log &) = delete;
  Log &operator=(const Log &) = delete;

  ~Log() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      stop_thread_ = true;
      cond_var_.notify_all();
    }
    log_thread_.join();
    if (file_ != nullptr) {
      std::fflush(file_); // flush that buffer
      std::fclose(file_);
    }
  }

  void PrintV(const char *prefix, const char *format, std::va_list va) {
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
    new_format.append(prefix);
    new_format.append(format);
    new_format.append("\n");

    char buffer[1024];
    std::vsnprintf(buffer, sizeof(buffer), new_format.c_str(), va);

    {
      std::lock_guard<std::mutex> lock(mutex_);
      log_queue_.emplace(buffer);
    }
    cond_var_.notify_one();
  }

 private:
  void ProcessQueue() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!stop_thread_) {
      cond_var_.wait(lock, [this] { return !log_queue_.empty() || stop_thread_; });
      while (!log_queue_.empty()) {
        const std::string &log_entry = log_queue_.front();
        if (file_ != nullptr) {
          std::fwrite(log_entry.data(), 1, log_entry.size(), file_);
          std::fflush(file_); // flush that shit
        } else {
          logprintf("%s", log_entry.c_str());
        }
        log_queue_.pop();
      }
    }
  }

  std::FILE *file_;
  std::unique_ptr<char[]> buffer_;
  std::string time_format_;
  std::queue<std::string> log_queue_;
  std::mutex mutex_;
  std::condition_variable cond_var_;
  std::thread log_thread_;
  bool stop_thread_;
};

}

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

