// Copyright (c) 2011-2018 Zeex
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

#ifndef CRASHDETECTHANDLER_H
#define CRASHDETECTHANDLER_H

#include <cstdarg>
#include <cstdio>
#include <cstdio>
#include <thread>
#include <atomic>
#include "amxcallstack.h"
#include "amxdebuginfo.h"
#include "amxhandler.h"
#include "amxref.h"
#include "regexp.h"
#include <mutex>

class AMXPathFinder;
class AMXStackFrame;

namespace os {
  class Context;
}

class CrashDetectHandler: public AMXHandler<CrashDetectHandler> {
 public:
  friend class AMXHandler<CrashDetectHandler>; // for accessing private ctor

  void set_amx_path_finder(AMXPathFinder *finder) {
    amx_path_finder_ = finder;
  }

  int Load();
  int Unload();

  int ProcessDebugHook();
  int ProcessCallback(cell index, cell *result, cell *params);
  int ProcessExec(cell *retval, int index);
  void ProcessExecError(int index, cell *retval, int error);

 public:
  static void StartThread();
  static void StopThread();
  static void OnCrash(const os::Context &context);
  static void OnInterrupt(const os::Context &context);

  static void PrintAMXBacktrace();
  static void PrintAMXBacktrace(std::ostream &stream);

  static void PrintNativeBacktrace(const os::Context &context);
  static void PrintNativeBacktrace(std::ostream &stream,
                                   const os::Context &context);

 private:
  static void PrintTraceFrame(const AMXStackFrame &frame,
                              const AMXDebugInfo &debug_info);
  static void PrintRuntimeError(AMXRef amx, const AMX &amx_state, int error);
  static void PrintRegisters(const os::Context &context);
  static void PrintStack(const os::Context &context);
  static void PrintLoadedModules();

 private:
  CrashDetectHandler(AMX *amx);
  static void HangThread();

 private:
  AMXRef amx_;
  AMXPathFinder *amx_path_finder_;
  AMXDebugInfo debug_info_;
  AMX_DEBUG prev_debug_;
  AMX_CALLBACK prev_callback_;
  cell last_frame_;
  std::string amx_path_;
  std::string amx_name_;
  bool block_exec_errors_;

 private:
  static std::thread hang_thread_;
  static std::atomic<bool> running_;
  static std::mutex mutex_;
  static AMXCallStack call_stack_;
};

#endif // !CRASHDETECTHANDLER_H
