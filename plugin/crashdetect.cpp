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

#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

#include "amxcallstack.h"
#include "amxdebuginfo.h"
#include "amxerror.h"
#include "amxopcode.h"
#include "amxpathfinder.h"
#include "amxscript.h"
#include "amxstacktrace.h"
#include "configreader.h"
#include "crashdetect.h"
#include "fileutils.h"
#include "logprintf.h"
#include "os.h"
#include "stacktrace.h"
#include "utils.h"

#define AMX_EXEC_GDK (-10)

AMXCallStack CrashDetect::call_stack_;

CrashDetect::CrashDetect(AMX *amx)
 : AMXService<CrashDetect>(amx),
   prev_debug_(0),
   prev_callback_(0),
   last_frame_(amx->stp),
   block_exec_errors_(false),
   trace_flags_(TRACE_NONE)
{
}

int CrashDetect::Load() {
  AMXPathFinder amx_finder;
  amx_finder.AddSearchPath("gamemodes");
  amx_finder.AddSearchPath("filterscripts");

  const char *var = getenv("AMX_PATH");
  if (var != 0) {
    utils::SplitString(var, fileutils::kNativePathListSepChar,
        std::bind1st(std::mem_fun(&AMXPathFinder::AddSearchPath), &amx_finder));
  }

  amx_path_ = amx_finder.FindAmx(amx());
  if (!amx_path_.empty()) {
    if (AMXDebugInfo::IsPresent(amx())) {
      debug_info_.Load(amx_path_);
    }
  }

  amx_name_ = fileutils::GetFileName(amx_path_);
  if (amx_name_.empty()) {
    assert(amx_path_.empty());
    amx_name_ = "<unknown>";
  }

  amx().DisableSysreqD();
  prev_debug_ = amx().GetDebugHook();
  prev_callback_ = amx().GetCallback();

  ConfigReader server_cfg("server.cfg");

  std::string trace;
  server_cfg.GetOption("trace", trace);

  for (std::size_t i = 0; i < trace.length(); i++) {
    switch (trace[i]) {
      case 'n':
        trace_flags_ |= TRACE_NATIVES;
        break;
      case 'p':
        trace_flags_ |= TRACE_PUBLICS;
        break;
      case 'f':
        trace_flags_ |= TRACE_FUNCTIONS;
        break;
    }
  }

  return AMX_ERR_NONE;
}

int CrashDetect::Unload() {
  return AMX_ERR_NONE;
}

int CrashDetect::DoAmxDebug() {
  if (amx().GetFrm() < last_frame_ && (trace_flags_ & TRACE_FUNCTIONS)
      && debug_info_.IsLoaded()) {
    AMXStackTrace trace =
      GetAMXStackTrace(amx(), amx().GetFrm(), amx().GetCip(), 1);
    if (trace.current_frame().return_address() != 0) {
      PrintTraceFrame(trace.current_frame(), debug_info_);
    }
  }
  last_frame_ = amx().GetFrm();
  return prev_debug_ != 0 ? prev_debug_(amx()) : AMX_ERR_NONE;
}

int CrashDetect::DoAmxCallback(cell index, cell *result, cell *params) {
  call_stack_.Push(AMXCall::Native(amx(), index));

  if (trace_flags_ & TRACE_NATIVES) {
    TracePrint("native %s ()", amx().GetNativeName(index));
  }

  int error = prev_callback_(amx(), index, result, params);

  call_stack_.Pop();
  return error;
}

int CrashDetect::DoAmxExec(cell *retval, int index) {
  call_stack_.Push(AMXCall::Public(amx(), index));

  if (trace_flags_ & TRACE_FUNCTIONS) {
    last_frame_ = 0;
  }
  if (trace_flags_ & TRACE_PUBLICS) {
    if (cell address = amx().GetPublicAddress(index)) {
      AMXStackTrace trace =
        GetAMXStackTrace(amx(), amx().GetFrm(), amx().GetCip(), 1);
      AMXStackFrame frame = trace.current_frame();
      if (frame.return_address() != 0) {
        frame.set_caller_address(address);
        PrintTraceFrame(frame, debug_info_);
      } else {
        AMXStackFrame fake_frame(amx(), amx().GetFrm(), 0, 0, address);
        PrintTraceFrame(fake_frame, debug_info_);
      }
    }
  }

  int error = ::amx_Exec(amx(), retval, index);
  if (error == AMX_ERR_CALLBACK ||
      error == AMX_ERR_NOTFOUND ||
      error == AMX_ERR_INIT     ||
      error == AMX_ERR_INDEX    ||
      error == AMX_ERR_SLEEP)
  {
    // For these types of errors amx_Error() is not called because of
    // early return from amx_Exec().
    HandleExecError(index, retval, error);
  }

  call_stack_.Pop();
  return error;
}

void CrashDetect::HandleExecError(int index,
                                  cell *retval,
                                  const AMXError &error) {
  if (block_exec_errors_) {
    return;
  }

  // The following error codes should not be treated as errors:
  //
  // 1. AMX_ERR_NONE is always returned when a public function returns
  //    normally as it jumps to return address 0 where it performs "halt 0".
  //
  // 2. AMX_ERR_SLEEP is returned when the VM is put into sleep mode. The
  //    execution can be later continued using AMX_EXEC_CONT.
  if (error.code() == AMX_ERR_NONE || error.code() == AMX_ERR_SLEEP) {
    return;
  }

  // For compatibility with sampgdk.
  if (error.code() == AMX_ERR_INDEX && index == AMX_EXEC_GDK) {
    return;
  }

  // Block errors while calling OnRuntimeError as it may result in yet
  // another error (and for certain errors it in fact always does, e.g.
  // stack/heap collision due to insufficient stack space for making
  // the public call).
  block_exec_errors_ = true;

  // Capture backtrace before continuing as OnRuntimError will modify the
  // state of the AMX thus we'll end up with a different stack and possibly
  // other things too. This also should protect from cases where something
  // hooks logprintf (like fixes2).
  std::stringstream bt_stream;
  PrintAmxBacktrace(bt_stream);

  // public OnRuntimeError(code, &bool:suppress);
  cell callback_index = amx().GetPublicIndex("OnRuntimeError");
  cell suppress = 0;

  if (callback_index >= 0) {
    if (amx().IsStackOK()) {
      cell suppress_addr, *suppress_ptr;
      amx_PushArray(amx(), &suppress_addr, &suppress_ptr, &suppress, 1);
      amx_Push(amx(), error.code());
      DoAmxExec(retval, callback_index);
      amx_Release(amx(), suppress_addr);
      suppress = *suppress_ptr;
    }
  }

  if (suppress == 0) {
    PrintRuntimeError(amx(), error);
    if (error.code() != AMX_ERR_NOTFOUND &&
        error.code() != AMX_ERR_INDEX    &&
        error.code() != AMX_ERR_CALLBACK &&
        error.code() != AMX_ERR_INIT) {
      DebugPrint(bt_stream.str().c_str());
    }
  }

  block_exec_errors_ = false;
}

void CrashDetect::HandleException() {
  DebugPrint("Server crashed while executing %s", amx_name_.c_str());
  PrintAmxBacktrace();
}

void CrashDetect::HandleInterrupt() {
  DebugPrint("Server received interrupt signal while executing %s",
             amx_name_.c_str());
  PrintAmxBacktrace();
}

// static
bool CrashDetect::IsInsideAmx() {
  return !call_stack_.IsEmpty();
}

// static
void CrashDetect::OnException(void *context) {
  if (IsInsideAmx()) {
    CrashDetect::GetInstance(call_stack_.Top().amx())->HandleException();
  } else {
    DebugPrint("Server crashed due to an unknown error");
  }
  PrintNativeBacktrace(context);
}

// static
void CrashDetect::OnInterrupt(void *context) {
  if (IsInsideAmx()) {
    CrashDetect::GetInstance(call_stack_.Top().amx())->HandleInterrupt();
  } else {
    DebugPrint("Server received interrupt signal");
  }
  PrintNativeBacktrace(context);
}

namespace {

void Replace(const char *s, char c, const char *r, std::string& result) {
  std::size_t len = std::strlen(s);
  for (std::size_t i = 0; i < len; i++) {
    result += s[i];
    if (s[i] == c) {
      result.append(r);
    }
  }
}

void Print(const char *prefix, const char *format, va_list va) {
  std::string new_format(prefix);
  Replace(format, '\n', prefix, new_format);
  vlogprintf(new_format.c_str(), va);
}

} // anonymous namespace

// static
void CrashDetect::TracePrint(const char *format, ...) {
  std::va_list va;
  va_start(va, format);
  Print("[trace] ", format, va);
  va_end(va);
}

// static
void CrashDetect::DebugPrint(const char *format, ...) {
  std::va_list va;
  va_start(va, format);
  Print("[debug] ", format, va);
  va_end(va);
}

// static
void CrashDetect::PrintTraceFrame(const AMXStackFrame &frame,
                                  const AMXDebugInfo &debug_info) {
  std::stringstream stream;
  AMXStackFramePrinter printer(stream, debug_info);
  printer.PrintCallerNameAndArguments(frame);
  TracePrint(stream.str().c_str());
}

// static
void CrashDetect::PrintRuntimeError(AMXScript amx, const AMXError &error) {
  DebugPrint("Run time error %d: \"%s\"", error.code(), error.GetString());
  cell *ip = reinterpret_cast<cell*>(amx.GetCode() + amx.GetCip());
  switch (error.code()) {
    case AMX_ERR_BOUNDS: {
      cell opcode = *ip;
      if (opcode == RelocateAmxOpcode(AMX_OP_BOUNDS)) {
        cell bound = *(ip + 1);
        cell index = amx.GetPri();
        if (index < 0) {
          DebugPrint(" Accessing element at negative index %d", index);
        } else {
          DebugPrint(" Accessing element at index %d past array upper bound %d",
                     index, bound);
        }
      }
      break;
    }
    case AMX_ERR_NOTFOUND: {
      const AMX_FUNCSTUBNT *natives = amx.GetNatives();
      int num_natives = amx.GetNumNatives();
      for (int i = 0; i < num_natives; ++i) {
        if (natives[i].address == 0) {
          DebugPrint(" %s", amx.GetName(natives[i].nameofs));
        }
      }
      break;
    }
    case AMX_ERR_STACKERR:
      DebugPrint(" Stack pointer (STK) is 0x%X, heap pointer (HEA) is 0x%X",
                 amx.GetStk(), amx.GetHea());
      break;
    case AMX_ERR_STACKLOW:
      DebugPrint(" Stack pointer (STK) is 0x%X, stack top (STP) is 0x%X",
                 amx.GetStk(), amx.GetStp());
      break;
    case AMX_ERR_HEAPLOW:
      DebugPrint(" Heap pointer (HEA) is 0x%X, heap bottom (HLW) is 0x%X",
                 amx.GetHea(), amx.GetHlw());
      break;
    case AMX_ERR_INVINSTR: {
      cell opcode = *ip;
      DebugPrint(" Unknown opcode 0x%x at address 0x%08X",
                 opcode , amx.GetCip());
      break;
    }
    case AMX_ERR_NATIVE: {
      cell opcode = *(ip - 2);
      if (opcode == RelocateAmxOpcode(AMX_OP_SYSREQ_C)) {
        cell index = *(ip - 1);
        DebugPrint(" %s", amx.GetNativeName(index));
      }
      break;
    }
  }
}

// static
void CrashDetect::PrintAmxBacktrace() {
  std::stringstream stream;
  PrintAmxBacktrace(stream);
  DebugPrint(stream.str().c_str());
}

namespace {

class HexDword {
 public:
  static const int kWidth = 8;
  HexDword(uint32_t value): value_(value) {}
  HexDword(void *value): value_(reinterpret_cast<uint32_t>(value)) {}
  friend std::ostream &operator<<(std::ostream &stream, const HexDword &x) {
    char fill = stream.fill();
    stream << std::setw(kWidth) << std::setfill('0') << std::hex
           << x.value_ << std::dec;
    stream.fill(fill);
    return stream;
  }
 private:
  uint32_t value_;
};

} // anonymous namespace

// static
void CrashDetect::PrintAmxBacktrace(std::ostream &stream) {
  AMXScript amx = call_stack_.Top().amx();
  AMXScript top_amx = amx;

  stream << "AMX backtrace:";

  AMXCallStack calls = call_stack_;

  cell cip = top_amx.GetCip();
  cell frm = top_amx.GetFrm();
  int level = 0;

  while (!calls.IsEmpty() && cip != 0 && amx == top_amx) {
    AMXCall call = calls.Pop();

    // native function
    if (call.IsNative()) {
      cell address = amx.GetNativeAddress(call.index());
      if (address != 0) {
        stream << "\n#" << level++ << " native ";

        const char *name = amx.GetNativeName(call.index());
        if (name != 0) {
          stream << name;
        } else {
          stream << "<unknown>";
        }

        char fill = stream.fill();
        stream << " () [" << HexDword(address) << "]";
        stream.fill(fill);

        void *ptr = reinterpret_cast<void*>(address);
        std::string path = os::GetModuleName(ptr);
        std::string module = fileutils::GetFileName(path);
        if (!module.empty()) {
          stream << " from " << module;
        }
      }
    }

    // public function
    else if (call.IsPublic()) {
      CrashDetect *cd = CrashDetect::GetInstance(amx);

      AMXStackTrace trace = GetAMXStackTrace(amx, frm, cip, 100);
      std::deque<AMXStackFrame> frames;

      while (trace.current_frame().return_address() != 0) {
        frames.push_back(trace.current_frame());
        if (!trace.MoveNext()) {
          break;
        }
      }

      cell entry_point = amx.GetPublicAddress(call.index());
      if (frames.empty()) {
        AMXStackFrame fake_frame(amx, frm, 0, 0, entry_point);
        frames.push_front(fake_frame);
      } else {
        frames.back().set_caller_address(entry_point);
      }

      for (std::deque<AMXStackFrame>::const_iterator it = frames.begin();
           it != frames.end(); it++) {
        const AMXStackFrame &frame = *it;

        stream << "\n#" << level++ << " ";

        const AMXDebugInfo &debug_info = cd->debug_info_;
        frame.Print(stream, debug_info);

        if (!debug_info.IsLoaded()) {
          const std::string &amx_name = cd->amx_name_;
          if (!amx_name.empty()) {
            stream << " from " << amx_name;
          }
        }
      }

      frm = call.frm();
      cip = call.cip();
    }
  }
}

// static
void CrashDetect::PrintNativeBacktrace(void *context) {
  std::stringstream stream;
  PrintNativeBacktrace(stream, context);
  DebugPrint(stream.str().c_str());
}

// static
void CrashDetect::PrintNativeBacktrace(std::ostream &stream, void *context) {
  StackTrace trace(context);
  std::deque<StackFrame> frames = trace.GetFrames();

  if (frames.empty()) {
    return;
  }

  stream << "Native backtrace:";

  int level = 0;
  for (std::deque<StackFrame>::const_iterator it = frames.begin();
       it != frames.end(); it++) {
    const StackFrame &frame = *it;

    stream << "\n#" << level++ << " ";
    frame.Print(stream);

    std::string module = os::GetModuleName(frame.return_address());
    if (!module.empty()) {
      stream << " from " << fileutils::GetRelativePath(module);
    }
  }
}
