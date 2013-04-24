// Copyright (c) 2011-2013 Zeex
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
#include <deque>
#include <sstream>
#include <stack>
#include <string>

#include "amxdebuginfo.h"
#include "amxerror.h"
#include "amxpathfinder.h"
#include "amxscript.h"
#include "amxstacktrace.h"
#include "compiler.h"
#include "crashdetect.h"
#include "fileutils.h"
#include "logprintf.h"
#include "npcall.h"
#include "os.h"
#include "stacktrace.h"

#define AMX_EXEC_GDK (-10)

static const int kOpBounds  = 121;
static const int kOpSysreqC = 123;

bool CrashDetect::block_exec_errors_ = false;
std::stack<NPCall*> CrashDetect::np_calls_;

// static
void CrashDetect::OnException(void *context) {
  if (!np_calls_.empty()) {
    CrashDetect::Get(np_calls_.top()->amx())->HandleException();
  } else {
    Printf("Server crashed due to an unknown error");
  }
  PrintNativeBacktrace(context);
}

// static
void CrashDetect::OnInterrupt(void *context) {
  if (!np_calls_.empty()) {
    CrashDetect::Get(np_calls_.top()->amx())->HandleInterrupt();
  } else {
    Printf("Server received interrupt signal");
  }
  PrintNativeBacktrace(context);
}

// static
void CrashDetect::PrintAmxBacktrace() {
  if (np_calls_.empty()) {
    return;
  }

  AMXScript top_amx = np_calls_.top()->amx();

  if (top_amx.GetCip() == 0) {
    return;
  }

  Printf("AMX backtrace:");

  std::stack<NPCall*> np_calls = np_calls_;

  cell cip = top_amx.GetCip();
  cell frm = top_amx.GetFrm();
  int level = 0;

  while (!np_calls.empty() && cip != 0) {
    const NPCall *call = np_calls.top();
    AMXScript amx = call->amx();

    if (amx != top_amx) {
      assert(level != 0);
      break;
    }

    if (call->IsNative()) {
      AMX_NATIVE address = reinterpret_cast<AMX_NATIVE>(amx.GetNativeAddress(call->index()));
      if (address != 0) {
        const char *name = amx.GetNativeName(call->index());
        if (name == 0) {
          name = "<unknown>";
        }
        std::string module = fileutils::GetFileName(os::GetModulePathFromAddr((void*)address));
        std::string from;
        if (!module.empty()) {
          from.append(" from ");
          from.append(module);
        }
        Printf("#%d native %s () [%08x]%s", level++, name, address, from.c_str());
      }
    }
    else if (call->IsPublic()) {
      const AMXDebugInfo &debug_info = CrashDetect::Get(amx)->debug_info_;
      const std::string &amx_name = CrashDetect::Get(amx)->amx_name_;

      amx.PushStack(cip);
      amx.PushStack(frm);
      amx.SetFrm(amx.GetStk());

      AMXStackTrace trace(amx, amx.GetFrm());
      std::deque<AMXStackFrame> frames;

      if (trace.current_frame()) {
        do {
          AMXStackFrame frame = trace.current_frame();
          frames.push_back(frame);
        }
        while (trace.Next());
        frames.back().set_caller_address(amx.GetPublicAddress(call->index()));
      } else {
        cell entry_point = amx.GetPublicAddress(call->index());
        frames.push_front(AMXStackFrame(amx, amx.GetFrm(), 0, 0, entry_point));
      }

      frm = amx.PopStack();
      cip = amx.PopStack();
      amx.SetFrm(frm);

      for (std::deque<AMXStackFrame>::const_iterator iterator = frames.begin();
           iterator != frames.end(); iterator++) {
        const AMXStackFrame &frame = *iterator;

        std::stringstream stream;
        frame.Print(stream, &debug_info);

        if (!debug_info.IsLoaded() && !amx_name.empty()) {
          stream << " from " << amx_name;
        }

        Printf("#%d %s", level++, stream.str().c_str());
      }

      frm = call->frm();
      cip = call->cip();
    }

    np_calls.pop();
  }
}

// static
void CrashDetect::PrintNativeBacktrace(void *context) {
  StackTrace trace(context);
  std::deque<StackFrame> frames = trace.GetFrames();

  if (frames.empty()) {
    return;
  }

  Printf("Native backtrace:");

  int level = 0;
  for (std::deque<StackFrame>::const_iterator iterator = frames.begin();
      iterator != frames.end(); ++iterator) {
    const StackFrame &frame = *iterator;

    std::stringstream stream;

    frame.Print(stream);

    std::string module = os::GetModulePathFromAddr(frame.return_address());
    if (!module.empty()) {
      stream << " from " << module;
    }

    Printf("#%d %s", level++, stream.str().c_str());
  }
}

// static
void CrashDetect::Printf(const char *format, ...) {
  std::va_list va;
  va_start(va, format);

  std::string new_format;
  new_format.append("[debug] ");
  new_format.append(format);

  vlogprintf(new_format.c_str(), va);
  va_end(va);
}

CrashDetect::CrashDetect(AMX *amx)
 : AMXService<CrashDetect>(amx),
   amx_(amx),
   prev_callback_(0)
{

}

int CrashDetect::Load() {
  AMXPathFinder pathFinder;
  pathFinder.AddSearchPath("gamemodes");
  pathFinder.AddSearchPath("filterscripts");

  // Read a list of additional search paths from AMX_PATH.
  const char *AMX_PATH = getenv("AMX_PATH");
  if (AMX_PATH != 0) {
    std::string var(AMX_PATH);
    std::string path;
    std::string::size_type begin = 0;
    while (begin < var.length()) {
      std::string::size_type end = var.find(fileutils::kNativePathListSepChar, begin);
      if (end == std::string::npos) {
        end = var.length();
      }
      path.assign(var.begin() + begin, var.begin() + end);
      if (!path.empty()) {
        pathFinder.AddSearchPath(path);
      }
      begin = end + 1;
    }
  }

  amx_path_ = pathFinder.FindAMX(amx_);
  amx_name_ = fileutils::GetFileName(amx_path_);

  if (!amx_path_.empty() && AMXDebugInfo::IsPresent(amx_)) {
    debug_info_.Load(amx_path_);
  }

  amx_.DisableSysreqD();
  prev_callback_ = amx_.GetCallback();

  return AMX_ERR_NONE;
}

int CrashDetect::Unload() {
  return AMX_ERR_NONE;
}

int CrashDetect::DoAmxCallback(cell index, cell *result, cell *params) {
  NPCall call = NPCall::Native(amx_, index);
  np_calls_.push(&call);
  int error = prev_callback_(amx_, index, result, params);
  np_calls_.pop();
  return error;
}

int CrashDetect::DoAmxExec(cell *retval, int index) {  
  NPCall call = NPCall::Public(amx_, index);
  np_calls_.push(&call);

  int error = ::amx_Exec(amx_, retval, index);
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

  np_calls_.pop();
  return error;
}

void CrashDetect::HandleExecError(int index, cell *retval, const AMXError &error) {
  if (block_exec_errors_) {
    return;
  }

  // Block errors while calling OnRuntimeError as it may result in yet
  // another error (and for certain errors it in fact always does, e.g.
  // stack/heap collision due to insufficient stack space for making
  // the public call).
  block_exec_errors_ = true;

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

  // public OnRuntimeError(code, &bool:suppress);
  cell callback_index = amx_.GetPublicIndex("OnRuntimeError");
  cell suppress = 0;

  if (callback_index >= 0) {
    cell suppress_addr, *suppress_ptr;
    amx_PushArray(amx_, &suppress_addr, &suppress_ptr, &suppress, 1);
    amx_Push(amx_, error.code());
    amx_Exec(amx_, retval, callback_index);
    amx_Release(amx_, suppress_addr);
    suppress = *suppress_ptr;
  }

  if (suppress == 0) {
    PrintError(error);
    if (error.code() != AMX_ERR_NOTFOUND &&
      error.code() != AMX_ERR_INDEX    &&
      error.code() != AMX_ERR_CALLBACK &&
      error.code() != AMX_ERR_INIT)
    {
      PrintAmxBacktrace();
    }
  }

  block_exec_errors_ = false;
}

void CrashDetect::HandleException() {
  Printf("Server crashed while executing %s", amx_name_.c_str());
  PrintAmxBacktrace();
}

void CrashDetect::HandleInterrupt() {
  Printf("Server received interrupt signal while executing %s", amx_name_.c_str());
  PrintAmxBacktrace();
}

void CrashDetect::PrintError(const AMXError &error) {
  Printf("Run time error %d: \"%s\"", error.code(), error.GetString());

  switch (error.code()) {
    case AMX_ERR_BOUNDS: {
      const cell *ip = reinterpret_cast<const cell*>(amx_.GetCode() + amx_.GetCip());
      cell opcode = *ip;
      if (opcode == GetAmxOpcode(kOpBounds)) {
        cell bound = *(ip + 1);
        cell index = amx_.GetPri();
        if (index < 0) {
          Printf(" Accessing element at negative index %d", index);
        } else {
          Printf(" Accessing element at index %d past array upper bound %d", index, bound);
        }
      }
      break;
    }
    case AMX_ERR_NOTFOUND: {
      const AMX_FUNCSTUBNT *natives = amx_.GetNatives();
      int num_natives = amx_.GetNumNatives();
      for (int i = 0; i < num_natives; ++i) {
        if (natives[i].address == 0) {
          Printf(" %s", amx_.GetName(natives[i].nameofs));
        }
      }
      break;
    }
    case AMX_ERR_STACKERR:
      Printf(" Stack pointer (STK) is 0x%X, heap pointer (HEA) is 0x%X", amx_.GetStk(), amx_.GetHea());
      break;
    case AMX_ERR_STACKLOW:
      Printf(" Stack pointer (STK) is 0x%X, stack top (STP) is 0x%X", amx_.GetStk(), amx_.GetStp());
      break;
    case AMX_ERR_HEAPLOW:
      Printf(" Heap pointer (HEA) is 0x%X, heap bottom (HLW) is 0x%X", amx_.GetHea(), amx_.GetHlw());
      break;
    case AMX_ERR_INVINSTR: {
      cell opcode = *(reinterpret_cast<const cell*>(amx_.GetCode() + amx_.GetCip()));
      Printf(" Unknown opcode 0x%x at address 0x%08X", opcode , amx_.GetCip());
      break;
    }
    case AMX_ERR_NATIVE: {
      const cell *ip = reinterpret_cast<const cell*>(amx_.GetCode() + amx_.GetCip());
      cell opcode = *(ip - 2);
      if (opcode == GetAmxOpcode(kOpSysreqC)) {
        cell index = *(ip - 1);
        Printf(" %s", amx_.GetNativeName(index));
      }
      break;
    }
  }
}

cell CrashDetect::GetAmxOpcode(cell index) {
  #ifdef LINUX
    static cell *opcode_map_ = 0;
    if (opcode_map_ == 0) {
      uint16_t flags = amx_.GetFlags();
      amx_.SetFlags(flags | AMX_FLAG_BROWSE);
      amx_Exec(amx_, reinterpret_cast<cell*>(&opcode_map_), 0);
      amx_.SetFlags(flags);
    }
    return opcode_map_[index];
  #else
    return index;
  #endif
}
