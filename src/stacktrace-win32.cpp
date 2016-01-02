// Copyright (c) 2012-2015 Zeex
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

#include <cstddef>
#include <cstdlib>

#include <Windows.h>
#include <DbgHelp.h>

#include "stacktrace.h"

namespace {

const int kMaxSymbolNameLength = 128;

class DbgHelp {
 public:
  DbgHelp(HANDLE process = NULL)
   : module_(LoadLibrary("DbgHelp.dll")),
     process_(process),
     initialized_(false)
  {
    if (process_ == NULL) {
      process_ = GetCurrentProcess();
    }
    if (module_ != NULL) {
      InitFunctions();
      if (SymInitialize != 0) {
        initialized_ = SymInitialize(process_, NULL, TRUE) != FALSE;
      }
    }
  }

  ~DbgHelp() {
    if (module_ != NULL) {
      if (initialized_ && SymCleanup != 0) {
        SymCleanup(process_);
      }
      FreeLibrary(module_);
    }
  }

  typedef BOOL (WINAPI *SymInitializePtr)(
    HANDLE hProcess,
    PCSTR UserSearchPath,
    BOOL fInvadeProcess
  );
  SymInitializePtr SymInitialize;

  typedef BOOL (WINAPI *SymCleanupPtr)(HANDLE hProcess);
  SymCleanupPtr SymCleanup;

  typedef BOOL (WINAPI *SymFromAddrPtr)(
    HANDLE hProcess,
    DWORD64 Address,
    PDWORD64 Displacement,
    PSYMBOL_INFO Symbol
  );
  SymFromAddrPtr SymFromAddr;

  typedef DWORD (WINAPI *SymGetOptionsPtr)();
  SymGetOptionsPtr SymGetOptions;

  typedef BOOL (WINAPI *SymSetOptionsPtr)(DWORD SymOptions);
  SymSetOptionsPtr SymSetOptions;

  typedef BOOL (WINAPI *SymGetModuleInfo64Ptr)(
    HANDLE hProcess,
    DWORD64 dwAddr,
    PIMAGEHLP_MODULE64 ModuleInfo
  );
  SymGetModuleInfo64Ptr SymGetModuleInfo64;

  typedef BOOL (WINAPI *StackWalk64Ptr)(
    DWORD MachineType,
    HANDLE hProcess,
    HANDLE hThread,
    LPSTACKFRAME64 StackFrame,
    LPVOID ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
  );
  StackWalk64Ptr StackWalk64;

  bool is_loaded() const {
    return module_ != NULL;
  }

  bool is_initialized() const {
    return initialized_;
  }

 private:
  DbgHelp(const DbgHelp &);
  void operator=(const DbgHelp &);

  void InitFunctions() {
    #define INIT_FUNC(Name) Name = (Name##Ptr)GetProcAddress(module_, #Name);
    INIT_FUNC(SymInitialize);
    INIT_FUNC(SymFromAddr);
    INIT_FUNC(SymCleanup);
    INIT_FUNC(SymGetOptions);
    INIT_FUNC(SymSetOptions);
    INIT_FUNC(SymGetModuleInfo64);
    INIT_FUNC(StackWalk64);
    #undef INIT_FUNC
  }

 private:
  HMODULE module_;
  HANDLE process_;
  bool initialized_;
};

} // anonymous namespace

void GetStackTrace(std::vector<StackFrame> &frames, void *their_context) {
  PCONTEXT context = reinterpret_cast<PCONTEXT>(their_context);
  CONTEXT current_context;
  if (their_context == NULL) {
    RtlCaptureContext(&current_context);
    context = &current_context;
  }

  HANDLE process = GetCurrentProcess();
  DbgHelp dbghelp(process);
  if (!dbghelp.is_loaded()) {
    return;
  }

  STACKFRAME64 stack_frame;
  ZeroMemory(&stack_frame, sizeof(stack_frame));

  // http://stackoverflow.com/a/136942/249230
  stack_frame.AddrPC.Offset = context->Eip;
  stack_frame.AddrReturn.Offset = context->Eip;
  stack_frame.AddrPC.Mode = AddrModeFlat;
  stack_frame.AddrFrame.Offset = context->Ebp;
  stack_frame.AddrFrame.Mode = AddrModeFlat;
  stack_frame.AddrStack.Offset = context->Esp;
  stack_frame.AddrStack.Mode = AddrModeFlat;

  if (!dbghelp.StackWalk64 != 0) {
    return;
  }

  SYMBOL_INFO *symbol = NULL;

  if (dbghelp.is_initialized()) {
    SIZE_T size = sizeof(SYMBOL_INFO) + kMaxSymbolNameLength + 1;
    DWORD flags = HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS;
    SYMBOL_INFO *symbol = static_cast<SYMBOL_INFO*>(HeapAlloc(GetProcessHeap(),
                                                              flags,
                                                              size));
    symbol->SizeOfStruct = sizeof(*symbol);
    symbol->MaxNameLen = kMaxSymbolNameLength;

    if (dbghelp.SymGetOptions != 0 && dbghelp.SymSetOptions != 0) {
      DWORD options = dbghelp.SymGetOptions();
      options |= SYMOPT_FAIL_CRITICAL_ERRORS;
      dbghelp.SymSetOptions(options);
    }
  }

  while (true) {
    DWORD64 address = stack_frame.AddrReturn.Offset;
    if (address == 0 || address & 0x80000000) {
      break;
    }

    bool have_symbols = true;
    if (dbghelp.SymGetModuleInfo64) {
      IMAGEHLP_MODULE64 module;
      ZeroMemory(&module, sizeof(IMAGEHLP_MODULE64));
      module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
      if (dbghelp.SymGetModuleInfo64(process, address, &module)) {
        if (!module.GlobalSymbols) {
          have_symbols = false;
        }
      }
    }

    const char *name = "";
    if (have_symbols) {
      if (dbghelp.is_initialized()
          && dbghelp.SymFromAddr != 0
          && symbol != 0) {
        if (dbghelp.SymFromAddr(process, address, NULL, symbol)) {
          name = symbol->Name;
        }
      }
    }

    frames.push_back(StackFrame(reinterpret_cast<void*>(address), name));

    if (!dbghelp.StackWalk64(IMAGE_FILE_MACHINE_I386,
                             process,
                             GetCurrentThread(),
                             &stack_frame,
                             (PVOID)context,
                             NULL,
                             NULL,
                             NULL,
                             NULL)) {
      break;
    }
  }

  HeapFree(GetProcessHeap(), 0, symbol);
  dbghelp.SymCleanup(process);
}
