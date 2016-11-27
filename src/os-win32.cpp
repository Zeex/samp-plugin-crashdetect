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

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#include "os.h"

namespace os {

Context::Registers Context::GetRegisters() const {
  Registers registers;
  if (native_context_ != 0) {
    PCONTEXT context = (PCONTEXT)native_context_;
    registers.eax = context->Eax;
    registers.ebx = context->Ebx;
    registers.ecx = context->Ecx;
    registers.edx = context->Edx;
    registers.esi = context->Esi;
    registers.edi = context->Edi;
    registers.ebp = context->Ebp;
    registers.esp = context->Esp;
    registers.eip = context->Eip;
    registers.eflags = context->EFlags;
  }
  return registers;
}

void GetLoadedModules(std::vector<Module> &modules) {
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
  if (snapshot != INVALID_HANDLE_VALUE) {
    modules.clear();
    MODULEENTRY32 module_entry;
    module_entry.dwSize = sizeof(module_entry);
    if (Module32First(snapshot, &module_entry)) {
      do {
        Module module(module_entry.szModule,
                      (uint32_t)module_entry.modBaseAddr,
                      module_entry.modBaseSize);
        modules.push_back(module);
        module_entry.dwSize = sizeof(module_entry);
      } while (Module32Next(snapshot, &module_entry));
    }
    CloseHandle(snapshot);
  }
}

std::string GetModuleName(void *address) {
  std::vector<char> filename(MAX_PATH);
  if (address != 0) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(address, &mbi, sizeof(mbi)) != 0) {
      DWORD size = filename.size();
      do {
        size = GetModuleFileName((HMODULE)mbi.AllocationBase,
                                 &filename[0], filename.size());
        if (size < filename.size() ||
            GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
          break;
        }
        filename.resize(size *= 2);
      } while (true);
    }
  }
  return std::string(&filename[0]);
}

namespace {

CrashHandler crash_handler = 0;
LPTOP_LEVEL_EXCEPTION_FILTER prev_except_handler;

LONG WINAPI ExceptionFilter(LPEXCEPTION_POINTERS exception) {
  if (crash_handler != 0) {
    crash_handler(Context(exception->ContextRecord));
  }
  if (prev_except_handler != 0) {
    return prev_except_handler(exception);
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

} // namespace

void SetCrashHandler(CrashHandler handler) {
  crash_handler = handler;
  if (handler != 0) {
    prev_except_handler = SetUnhandledExceptionFilter(ExceptionFilter);
  } else {
    SetUnhandledExceptionFilter(prev_except_handler);
  }
}

namespace {

InterruptHandler interrupt_handler;

struct ThreadInfo {
  DWORD    id;
  FILETIME creation_time;
  FILETIME exit_time;
  FILETIME kernel_time;
  FILETIME user_time;
};

class IsInvalidThread: std::unary_function<ThreadInfo, bool> {
 public:
  bool operator()(const ThreadInfo &thread) {
    return thread.creation_time.dwHighDateTime == 0 &&
           thread.creation_time.dwHighDateTime == 0;
  }
};

class CompareThreads: std::binary_function<ThreadInfo, ThreadInfo, bool> {
 public:
  bool operator()(const ThreadInfo &lhs, const ThreadInfo &rhs) {
    return CompareFileTime(&lhs.creation_time, &rhs.creation_time) < 0;
  }
};

HANDLE GetThreadHandle(DWORD thread_id, DWORD desired_access) {
  return OpenThread(desired_access, FALSE, thread_id);
}

DWORD GetMainThreadId() {
  std::vector<ThreadInfo> threads;

  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snapshot != INVALID_HANDLE_VALUE) {
    THREADENTRY32 thread_entry;
    thread_entry.dwSize = sizeof(thread_entry);
    if (Thread32First(snapshot, &thread_entry)) {
      DWORD process_id = GetProcessId(GetCurrentProcess());
      do {
        if (thread_entry.dwSize >=
            FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) +
                         sizeof(thread_entry.th32OwnerProcessID)) {
          if (thread_entry.th32OwnerProcessID == process_id) {
            ThreadInfo thread = {0};
            thread.id = thread_entry.th32ThreadID;
            threads.push_back(thread);
          }
        }
        thread_entry.dwSize = sizeof(thread_entry);
      } while (Thread32Next(snapshot, &thread_entry));
    }
    CloseHandle(snapshot);
  }

  for (std::vector<ThreadInfo>::iterator it = threads.begin();
       it != threads.end(); it++) {
    ThreadInfo &info = *it;
    HANDLE handle = GetThreadHandle(info.id, THREAD_QUERY_INFORMATION);
    GetThreadTimes(handle, &info.creation_time, &info.exit_time,
                           &info.kernel_time,   &info.user_time);
  }

  threads.erase(std::remove_if(threads.begin(), threads.end(),
                               IsInvalidThread()), threads.end());
  if (!threads.empty()) {
    return std::min_element(threads.begin(), threads.end(),
                            CompareThreads())->id;
  }
  return 0;
}

BOOL GetMainThreadContext(PCONTEXT context) {
  DWORD thread_id = GetMainThreadId();
  if (thread_id != 0) {
    HANDLE thread_handle = GetThreadHandle(thread_id, THREAD_GET_CONTEXT |
                                                      THREAD_SUSPEND_RESUME);
    if (thread_handle != NULL) {
      if (SuspendThread(thread_handle) != (DWORD)-1) {
        BOOL ok = GetThreadContext(thread_handle, context) != 0;
        ResumeThread(thread_handle);
        return ok;
      }
    }
  }
  return FALSE;
}

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
  switch (dwCtrlType) {
  case CTRL_C_EVENT:
    if (interrupt_handler != 0) {
      CONTEXT context = {0};
      context.ContextFlags = CONTEXT_FULL;
      GetMainThreadContext(&context);
      interrupt_handler(Context(&context));
    }
  }
  return FALSE;
}

} // namespace

void SetInterruptHandler(InterruptHandler handler) {
  interrupt_handler = handler;
  SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

} // namespace os
