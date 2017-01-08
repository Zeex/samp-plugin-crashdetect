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

#ifndef OS_H
#define OS_H

#include <string>
#include <vector>

#if defined __GNUC__ || (defined _MSC_VER && _MSC_VER >= 1600)
  #include <stdint.h>
  namespace os {
    using ::int32_t;
    using ::uint32_t;
  }
#elif defined _WIN32
  namespace os {
    typedef signed __int32 int32_t;
    typedef unsigned __int32 uint32_t;
  }
#endif

namespace os {

class Context;

typedef void (*CrashHandler)(const Context &context);
typedef void (*InterruptHandler)(const Context &context);

class Context {
 public:
  struct Registers {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t eflags;
  };

  Context(): native_context_(0) {}
  Context(void *native_context): native_context_(native_context) {}

  void *native_context() const {
    return native_context_;
  }

  Registers GetRegisters() const;

 private:
  Context(const Context &);
  void operator=(const Context &);

 private:
  void *native_context_;
};

class Module {
 public:
  Module(const std::string &name, uint32_t base_address, uint32_t size)
    : name_(name),
      base_address_(base_address),
      size_(size)
  {
  }

  const std::string &name() const {
    return name_;
  }

  uint32_t base_address() const {
    return base_address_;
  }

  uint32_t size() const {
    return size_;
  }

 private:
  std::string name_;
  uint32_t base_address_;
  uint32_t size_;
};

void GetLoadedModules(std::vector<Module> &modules);
std::string GetModuleName(void *address);

void SetCrashHandler(CrashHandler handler);
void SetInterruptHandler(InterruptHandler handler);

} // namespace os

#endif // !OS_H
