// Copyright (c) 2011-2014 Zeex
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

#include <string>

#include "amxcallstack.h"
#include "amxdebuginfo.h"
#include "amxscript.h"
#include "amxservice.h"

class AMXError;

class CrashDetect : public AMXService<CrashDetect> {
 friend class AMXService<CrashDetect>;

 public:
  int Load();
  int Unload();

  int DoAmxCallback(cell index, cell *result, cell *params);
  int DoAmxExec(cell *retval, int index);

  void HandleException();
  void HandleInterrupt();
  void HandleExecError(int index, cell *retval, const AMXError &error);

  static bool IsInsideAmx();

  static void OnException(void *context);
  static void OnInterrupt(void *context);

  static void Printf(const char *format, ...) ;
  static void PrintLines(std::string string);

  static void PrintRuntimeError(AMXScript amx, const AMXError &error);

  static void PrintAmxBacktrace();
  static void PrintAmxBacktrace(std::ostream &stream);

  static void PrintNativeBacktrace(void *context);
  static void PrintNativeBacktrace(std::ostream &stream, void *context);

 private:
  explicit CrashDetect(AMX *amx);

 private:
  AMXDebugInfo debug_info_;
  AMX_CALLBACK prev_callback_;

  std::string amx_path_;
  std::string amx_name_;

  bool block_exec_errors_;

 private:
  static AMXCallStack call_stack_;
};

#endif // !CRASHDETECT_H
