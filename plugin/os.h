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

#ifndef OS_H
#define OS_H

#include <cstddef>
#include <cstdio>
#include <string>

#include "cstdint.h"

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable: 4351) // new behavior: elements of array 'X'
                                 // will be default initialized
#endif

namespace os {

class BasicContext {
 public:
  enum Register {
    EAX, ECX, EDX, EBX, ESI, EDI, ESP, EBP, EIP, EFLAGS,
    NUM_REGISTERS
  };
  explicit BasicContext(): registers_() {}
  std::int32_t GetRegister(Register reg) const { return registers_[reg]; }
 protected:
  std::int32_t registers_[NUM_REGISTERS];
};

class Context;

// GetModulePathFromAddr finds which module (executable/DLL) a given 
// address belongs to.
std::string GetModulePathFromAddr(void *address,
                                  std::size_t max_length = FILENAME_MAX);

typedef void (*ExceptionHandler)(const Context &context);

// SetExceptionHandler sets a global exception handler on Windows and SIGSEGV
// signal handler on Linux.
void SetExceptionHandler(ExceptionHandler handler);

typedef void (*InterruptHandler)(const Context &context);

// SetInterruptHandler sets a global Ctrl+C event handler on Windows
// and SIGINT signal handler on Linux.
void SetInterruptHandler(InterruptHandler handler);

} // namespace os

#ifdef _WIN32
  #include "os-win32.h"
#else
  #include "os-unix.h"
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // !OS_H
