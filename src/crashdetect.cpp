// Copyright (c) 2011-2021 Zeex
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
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <amx/amxaux.h>
#include "amxcallstack.h"
#include "amxdebuginfo.h"
#include "amxopcode.h"
#include "amxpathfinder.h"
#include "amxref.h"
#include "amxstacktrace.h"
#include "crashdetect.h"
#include "fileutils.h"
#include "log.h"
#include "options.h"
#include "os.h"
#include "stacktrace.h"
#include "stringutils.h"

#define AMX_EXEC_GDK    (-10)
#define AMX_EXEC_GDK_42 (-10000)

namespace {

template<typename Printer>
class PrintLine: public std::unary_function<const std::string &, void> {
 public:
  PrintLine(Printer printer) : printer_(printer) {}
  void operator()(const std::string &line) {
    printer_("%s", line.c_str());
  }
 private:
  Printer printer_;
};

template<typename Printer>
void PrintStream(Printer printer, const std::stringstream &stream) {
  stringutils::SplitString(stream.str(),
                           '\n',
                           PrintLine<Printer>(printer));
}

} // anonymous namespace

AMXCallStack CrashDetect::call_stack_;

unsigned int CrashDetect::long_call_time_;
std::chrono::microseconds CrashDetect::long_call_time_current_;
std::chrono::high_resolution_clock::time_point CrashDetect::long_call_time_next_;
bool CrashDetect::long_call_time_running_;

CrashDetect::CrashDetect(AMX *amx)
  : AMXHandler<CrashDetect>(amx),
    amx_(amx),
    prev_debug_(nullptr),
    prev_callback_(nullptr),
    last_frame_(amx->stp),
    block_exec_errors_(false),
    address_naught_(false)
{
}

void CrashDetect::PluginLoad() {
  long_call_time_ = Options::shared().long_call_time();
  long_call_time_current_ = std::chrono::microseconds(long_call_time_);
  long_call_time_next_ = std::chrono::high_resolution_clock::time_point::max();
  long_call_time_running_ = long_call_time_ != 0;
}

void CrashDetect::PluginUnload() {
  long_call_time_running_ = false;
}

int CrashDetect::Load() {
  amx_path_ = AMXPathFinder::shared().Find(amx());
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

  amx_.SetSysreqDEnabled(false);
  prev_debug_ = amx_.GetDebugHook();
  prev_callback_ = amx_.GetCallback();

  return AMX_ERR_NONE;
}

int CrashDetect::Unload() {
  return AMX_ERR_NONE;
}

int CrashDetect::OnDebugHook() {
  if (amx_.GetFrm() < last_frame_
      && (Options::shared().trace_flags() & TRACE_FUNCTIONS)
      && debug_info_.IsLoaded()) {
    AMXStackTrace trace = GetAMXStackTrace(
      amx_,
      amx_.GetFrm(),
      amx_.GetCip(),
      1);
    if (trace.current_frame().return_address() != 0) {
      PrintTraceFrame(trace.current_frame(), debug_info_);
    }
  }
  last_frame_ = amx_.GetFrm();
  return prev_debug_ != nullptr ? prev_debug_(amx_) : AMX_ERR_NONE;
}

int CrashDetect::OnCallback(cell index, cell *result, cell *params) {
  Push(AMXCall::Native(amx_, index));

  if (Options::shared().trace_flags() & TRACE_NATIVES) {
    std::stringstream stream;
    const char *name = amx_.GetNativeName(index);
    stream << "native " << (name != nullptr ? name : "<unknown>") << " ()";
    if (Options::shared().trace_filter() == nullptr
        || Options::shared().trace_filter()->Test(stream.str())) {
      PrintStream(LogTracePrint, stream);
    }
  }

  int error = prev_callback_(amx_, index, result, params);

  Pop();
  return error;
}

int CrashDetect::OnExec(cell *retval, int index) {
  Push(AMXCall::Public(amx_, index));

  if (Options::shared().trace_flags() & TRACE_FUNCTIONS) {
    last_frame_ = 0;
  }
  if (Options::shared().trace_flags() & TRACE_PUBLICS) {
    if (cell address = amx_.GetPublicAddress(index)) {
      AMXStackTrace trace = GetAMXStackTrace(
        amx_,
        amx_.GetFrm(),
        amx_.GetCip(),
        1);
      AMXStackFrame frame = trace.current_frame();
      if (frame.return_address() != 0) {
        frame.set_caller_address(address);
        PrintTraceFrame(frame, debug_info_);
      } else {
        AMXStackFrame fake_frame(
          amx_,
          amx_.GetFrm(),
          0,
          0,
          address);
        PrintTraceFrame(fake_frame, debug_info_);
      }
    }
  }

  int error = ::amx_Exec(amx_, retval, index);
  if (error == AMX_ERR_CALLBACK
      || error == AMX_ERR_NOTFOUND
      || error == AMX_ERR_INIT
      || error == AMX_ERR_INDEX
      || error == AMX_ERR_SLEEP) {
    // For these types of errors amx_Error() is not called because of
    // early return from amx_Exec().
    OnExecError(index, retval, error);
  }

  Pop();
  return error;
}

int CrashDetect::OnExecError(int index, cell *retval, int error) {
  if (block_exec_errors_) {
    return AMX_ERR_NONE;
  }

  // The following error codes should not be treated as errors:
  //
  // 1. AMX_ERR_NONE is always returned when a public function returns
  //    normally as it jumps to return address 0 where it performs "halt 0".
  //
  // 2. AMX_ERR_SLEEP is returned when the VM is put into sleep mode. The
  //    execution can be later continued using AMX_EXEC_CONT.
  if (error == AMX_ERR_NONE || error == AMX_ERR_SLEEP) {
    return AMX_ERR_NONE;
  }

  // For compatibility with sampgdk.
  if (error == AMX_ERR_INDEX && (index == AMX_EXEC_GDK ||
                                 index <= AMX_EXEC_GDK_42)) {
    return AMX_ERR_NONE;
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
  PrintAMXBacktrace(bt_stream);

  // Remember values of AMX registers before calling OnRuntimeError().
  AMX amx_state = *amx_.amx();

  // public OnRuntimeError(code, &bool:suppress);
  cell callback_index = amx_.GetPublicIndex("OnRuntimeError");
  cell suppress = 0;

  if (callback_index >= 0) {
    if (amx_.CheckStack()) {
      cell suppress_addr, *suppress_ptr;
      amx_PushArray(amx_, &suppress_addr, &suppress_ptr, &suppress, 1);
      amx_Push(amx_, error);
      OnExec(retval, callback_index);
      amx_Release(amx_, suppress_addr);
      suppress = *suppress_ptr;
    }
  }

  if (suppress == 0) {
    PrintRuntimeError(amx_, amx_state, error);
    if (error != AMX_ERR_NOTFOUND
        && error != AMX_ERR_INDEX
        && error != AMX_ERR_CALLBACK
        && error != AMX_ERR_INIT) {
      PrintStream(LogDebugPrint, bt_stream);
    }
  }

  block_exec_errors_ = false;
  return AMX_ERR_NONE;
}

int CrashDetect::OnLongCallRequest(int option, int value) {
  if (long_call_time_ != 0) {
    switch (option) {
      case AMX_LCT_OPTION:
        return LongCallOption(value);
      case AMX_LCT_SET_TIME:
        SetLongCallTime(static_cast<unsigned int>(value));
        break;
      case AMX_LCT_CHECK:
        CheckLongCallTime();
        break;
    }
  }
  return AMX_ERR_NONE;
}

int CrashDetect::OnAddressNaughtRequest(int option) {
  switch (option) {
    case -1:
      return address_naught_;
    case 0:
      address_naught_ = false;
      break;
    case 1:
      address_naught_ = true;
      break;
  }
  return AMX_ERR_NONE;
}

// static
void CrashDetect::OnCrash(const os::Context &context) {
  CrashDetect *instance = nullptr;
  if (!call_stack_.IsEmpty()) {
    instance = GetHandler(call_stack_.Top().amx());
  }
  if (instance != nullptr) {
    LogDebugPrint("Server crashed while executing %s",\
                  instance->amx_name_.c_str());
  } else {
    LogDebugPrint("Server crashed due to an unknown error");
  }
  PrintAMXBacktrace();
  PrintNativeBacktrace(context.native_context());
  PrintRegisters(context);
  PrintStack(context);
  PrintLoadedModules();
}

// static
void CrashDetect::OnInterrupt(const os::Context &context) {
  CrashDetect *instance = nullptr;
  if (!call_stack_.IsEmpty()) {
    instance = GetHandler(call_stack_.Top().amx());
  }
  if (instance != nullptr) {
    LogDebugPrint("Server received interrupt signal while executing %s",
                  instance->amx_name_.c_str());
  } else {
    LogDebugPrint("Server received interrupt signal");
  }
  PrintAMXBacktrace();
  PrintNativeBacktrace(context.native_context());
}

// static
void CrashDetect::PrintTraceFrame(const AMXStackFrame &frame,
                                  const AMXDebugInfo &debug_info) {
  std::stringstream stream;
  AMXStackFramePrinter printer(stream, debug_info);
  printer.PrintCallerNameAndArguments(frame);
  if (Options::shared().trace_filter() == nullptr
      || Options::shared().trace_filter()->Test(stream.str())) {
    PrintStream(LogTracePrint, stream);
  }
}

// static
void CrashDetect::PrintRuntimeError(AMXRef amx,
                                    const AMX &amx_state,
                                    int error) {
  LogDebugPrint("Run time error %d: \"%s\"", error, aux_StrError(error));
  cell *ip = reinterpret_cast<cell*>(amx.GetCode() + amx_state.cip);
  switch (error) {
    case AMX_ERR_BOUNDS: {
      cell opcode = *ip;
      if (opcode == RelocateAMXOpcode(AMX_OP_BOUNDS)) {
        cell upper_bound = *(ip + 1);
        cell index = amx_state.pri;
        if (index < 0) {
          LogDebugPrint(" Attempted to read/write array element at negative "
                        "index %d", index);
        } else {
          LogDebugPrint(" Attempted to read/write array element at index %d "
                        "in array of size %d", index, upper_bound + 1);
        }
      }
      break;
    }
    case AMX_ERR_NOTFOUND: {
      const AMX_FUNCSTUBNT *natives = amx.GetNatives();
      int num_natives = amx.GetNumNatives();
      for (int i = 0; i < num_natives; ++i) {
        if (natives[i].address == 0) {
          LogDebugPrint(" %s", amx.GetString(natives[i].nameofs));
        }
      }
      break;
    }
    case AMX_ERR_STACKERR:
      LogDebugPrint(" Stack pointer (STK) is 0x%X, heap pointer (HEA) is 0x%X",
                    amx_state.stk, amx_state.hea);
      break;
    case AMX_ERR_STACKLOW:
      LogDebugPrint(" Stack pointer (STK) is 0x%X, stack top (STP) is 0x%X",
                    amx_state.stk, amx_state.stp);
      break;
    case AMX_ERR_HEAPLOW:
      LogDebugPrint(" Heap pointer (HEA) is 0x%X, heap bottom (HLW) is 0x%X",
                    amx_state.hea, amx_state.hlw);
      break;
    case AMX_ERR_INVINSTR: {
      cell opcode = *ip;
      LogDebugPrint(" Unknown opcode 0x%x at address 0x%08X",
                    opcode, amx_state.cip);
      break;
    }
    case AMX_ERR_NATIVE: {
      cell opcode = *(ip - 2);
      if (opcode == RelocateAMXOpcode(AMX_OP_SYSREQ_C)) {
        cell index = *(ip - 1);
        LogDebugPrint(" %s", amx.GetNativeName(index));
      }
      break;
    }
  }
}

// static
void CrashDetect::PrintAMXBacktrace() {
  std::stringstream stream;
  PrintAMXBacktrace(stream);
  PrintStream(LogDebugPrint, stream);
}

// static
void CrashDetect::PrintAMXBacktrace(std::ostream &stream) {
  if (call_stack_.IsEmpty()) {
    return;
  }

  AMXRef amx = call_stack_.Top().amx();
  AMXRef top_amx = amx;

  AMXCallStack calls = call_stack_;

  cell cip = top_amx.GetCip();
  cell frm = top_amx.GetFrm();
  int level = 0;

  if (!calls.IsEmpty() && cip != 0) {
    stream << "AMX backtrace:";
  }

  while (!calls.IsEmpty() && cip != 0 && amx == top_amx) {
    AMXCall call = calls.Pop();

    // native function
    if (call.IsNative()) {
      const char *name = amx.GetNativeName(call.index());
      stream << "\n#" << level++
             << " native "
             << (name != nullptr ? name : "<unknown>") << " ()";
      std::string module = os::GetModuleName(
        reinterpret_cast<void*>(amx.GetNativeAddress(call.index())));
      if (!module.empty()) {
        stream << " in " << fileutils::GetFileName(module);
      }
    }

    // public function
    else if (call.IsPublic()) {
      CrashDetect *handler = GetHandler(amx);

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
        frame.Print(stream, handler->debug_info_);

        if (!handler->debug_info_.IsLoaded()) {
          stream << " in " << handler->amx_name_;
        }
      }

      frm = call.frm();
      cip = call.cip();
    }
  }
}

// static
void CrashDetect::PrintRegisters(const os::Context &context) {
  os::Context::Registers registers = context.GetRegisters();

  LogDebugPrint("Registers:");
  LogDebugPrint("EAX: %08x EBX: %08x ECX: %08x EDX: %08x",
                registers.eax,
                registers.ebx,
                registers.ecx,
                registers.edx);
  LogDebugPrint("ESI: %08x EDI: %08x EBP: %08x ESP: %08x",
                registers.esi,
                registers.edi,
                registers.ebp,
                registers.esp);
  LogDebugPrint("EIP: %08x EFLAGS: %08x",
                registers.eip,
                registers.eflags);
}

// static
void CrashDetect::PrintStack(const os::Context &context) {
  os::Context::Registers registers = context.GetRegisters();
  os::uint32_t *stack_ptr = reinterpret_cast<os::uint32_t *>(registers.esp);
  if (stack_ptr == nullptr) {
    return;
  }

  LogDebugPrint("Stack:");

  for (int i = 0; i < 256; i += 8) {
    LogDebugPrint("ESP+%08x: %08x %08x %08x %08x",
                  i * sizeof(*stack_ptr),
                  stack_ptr[i],
                  stack_ptr[i + 1],
                  stack_ptr[i + 2],
                  stack_ptr[i + 3]);
  }
}

// static
void CrashDetect::PrintLoadedModules() {
  LogDebugPrint("Loaded modules:");

  std::vector<os::Module> modules;
  os::GetLoadedModules(modules);

  for (std::vector<os::Module>::const_iterator it = modules.begin();
       it != modules.end(); it++) {
    const os::Module &module = *it;
    LogDebugPrint("%08x - %08x %s",
                  module.base_address(),
                  module.base_address() + module.size(),
                  module.name().c_str());
  }
}

// static
void CrashDetect::Push(AMXCall call) {
  if (call_stack_.IsEmpty()) {
    long_call_time_next_ =
        std::chrono::high_resolution_clock::now() + long_call_time_current_;
  }
  call_stack_.Push(call);
}

// static
AMXCall CrashDetect::Pop() {
  AMXCall call = call_stack_.Pop();
  if (call_stack_.IsEmpty()) {
    long_call_time_next_ =
        std::chrono::high_resolution_clock::time_point::max();
  }
  return call;
}

void CrashDetect::PrintNativeBacktrace(const os::Context &context) {
  std::stringstream stream;
  PrintNativeBacktrace(stream, context);
  PrintStream(LogDebugPrint, stream);
}

// static
void CrashDetect::PrintNativeBacktrace(std::ostream &stream,
                                       const os::Context &context) {
  std::vector<StackFrame> frames;
  GetStackTrace(frames, context.native_context());

  if (!frames.empty()) {
    stream << "Native backtrace:";

    int level = 0;
    for (std::vector<StackFrame>::const_iterator it = frames.begin();
         it != frames.end(); it++) {
      const StackFrame &frame = *it;

      stream << "\n#" << level++ << " ";
      frame.Print(stream);

      std::string module = os::GetModuleName(frame.return_address());
      if (!module.empty()) {
        stream << " in " << fileutils::GetRelativePath(module);
      }
    }
  }
}

// static
void CrashDetect::SetLongCallTime(unsigned int time) {
  long_call_time_current_ = std::chrono::microseconds(time);
}

// static
unsigned int CrashDetect::LongCallOption(int option) {
  switch (option) {
    case AMX_LCT_OPTION_CURRENT:
      return static_cast<unsigned int>(long_call_time_current_.count());
    // case AMX_LCT_OPTION_ORIGINAL:
    //   return CrashDetect::long_call_time_;
    case AMX_LCT_OPTION_ACTIVE:
      return long_call_time_running_;
    case AMX_LCT_OPTION_RESTART:
      long_call_time_next_ =
          std::chrono::high_resolution_clock::now() + long_call_time_current_;
      break;
    case AMX_LCT_OPTION_DISABLE:
      long_call_time_running_ = false;
      break;
    case AMX_LCT_OPTION_ENABLE:
      long_call_time_running_ = long_call_time_ != 0;
      break;
    case AMX_LCT_OPTION_RESET:
      SetLongCallTime(long_call_time_);
      break;
  }
  return 0;
}

// static
void CrashDetect::CheckLongCallTime(void) {
  if (!long_call_time_running_) {
    return;
  }
  if (long_call_time_next_ < std::chrono::high_resolution_clock::now()) {
    // Disable repeat stack dumps by setting this WAY in the future.
    long_call_time_next_ =
        std::chrono::high_resolution_clock::time_point::max();
    LogDebugPrint("Long callback execution detected (hang or performance issue)");
    PrintAMXBacktrace();
  }
}
