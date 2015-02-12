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

#include <Windows.h>
#include <DbgHelp.h>

#include "stacktrace.h"

static const int kMaxSymbolNameLength = 128;

void GetStackTrace(std::vector<StackFrame> &frames, void *their_context) {
  PCONTEXT context = reinterpret_cast<PCONTEXT>(their_context);
  CONTEXT current_context;
  if (context == NULL) {
    RtlCaptureContext(&current_context);
    context = &current_context;
  }

  HANDLE process = GetCurrentProcess();
  if (!SymInitialize(process, NULL, TRUE)) {
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

  SIZE_T size = sizeof(SYMBOL_INFO) + kMaxSymbolNameLength + 1;
  DWORD flags = HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS;
  SYMBOL_INFO *symbol = static_cast<SYMBOL_INFO*>(HeapAlloc(GetProcessHeap(),
                                                            flags, size));
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = kMaxSymbolNameLength;

  DWORD options = SymGetOptions();
  options |= SYMOPT_FAIL_CRITICAL_ERRORS;
  SymSetOptions(options);

  while (true) {
    DWORD64 address = stack_frame.AddrReturn.Offset;
    if (address == 0 || address & 0x80000000) {
      break;
    }

    IMAGEHLP_MODULE64 module;
    ZeroMemory(&module, sizeof(IMAGEHLP_MODULE64));
    module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    SymGetModuleInfo64(process, address, &module);

    PTSTR name = TEXT("");
    if (module.GlobalSymbols &&
        SymFromAddr(process, address, NULL, symbol)) {
      name = symbol->Name;
    }

    frames.push_back(StackFrame(reinterpret_cast<void*>(address), name));

    HANDLE thread = GetCurrentThread();
    if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, thread, &stack_frame,
                     (PVOID)context, NULL, NULL, NULL, NULL)) {
      break;
    }
  }

  HeapFree(GetProcessHeap(), 0, symbol);
  SymCleanup(process);
}
