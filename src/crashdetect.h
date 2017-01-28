// Copyright (c) 2011-2015 Zeex
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

#ifndef CRASHDETECT_H
#define CRASHDETECT_H

#include <cstdarg>
#include <cstdio>
#include <string>

#include "amxcallstack.h"
#include "amxdebuginfo.h"
#include "amxscript.h"
#include "amxservice.h"
#include "regexp.h"

class AMXError;
class AMXStackFrame;

namespace os {
  class Context;
}

class CrashDetect : public AMXService<CrashDetect> {
 friend class AMXService<CrashDetect>;

 public:
  enum TraceFlags {
    TRACE_NONE = 0x00,
    TRACE_NATIVES = 0x01,
    TRACE_PUBLICS = 0x02,
    TRACE_FUNCTIONS = 0x04
  };

  int Load();
  int Unload();

  int HandleAMXDebug();
  int HandleAMXCallback(cell index, cell *result, cell *params);
  int HandleAMXExec(cell *retval, int index);
  void HandleAMXExecError(int index, cell *retval, const AMXError &error);

  static void OnCrash(const os::Context &context);
  static void OnInterrupt(const os::Context &context);

  static void PrintAMXBacktrace();
  static void PrintAMXBacktrace(std::ostream &stream);

  static void PrintNativeBacktrace(const os::Context &context);
  static void PrintNativeBacktrace(std::ostream &stream,
                                   const os::Context &context);

 private:
  void HandleException();
  void HandleInterrupt();

  static bool IsInsideAMX();

  static void PrintTraceFrame(const AMXStackFrame &frame,
                              const AMXDebugInfo &debug_info);

  static void PrintRuntimeError(AMXScript amx, const AMXError &error);

  static void PrintRegisters(const os::Context &context);
  static void PrintStack(const os::Context &context);
  static void PrintLoadedModules();

 private:
  CrashDetect(AMX *amx);

 private:
  AMXDebugInfo debug_info_;
  AMX_DEBUG prev_debug_;
  AMX_CALLBACK prev_callback_;
  cell last_frame_;
  std::string amx_path_;
  std::string amx_name_;
  bool block_exec_errors_;

 private:
  static int trace_flags_;
  static RegExp trace_filter_;
  static AMXCallStack call_stack_;
};

#endif // !CRASHDETECT_H
