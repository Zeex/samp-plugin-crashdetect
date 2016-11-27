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

#include <cassert>
#include <cstring>
#include <vector>

#include <dlfcn.h>
#include <link.h>
#include <signal.h>
#include <ucontext.h>

#include "os.h"

extern const char *__progname;

namespace os {

Context::Registers Context::GetRegisters() const {
  Registers registers = {0};
  if (native_context_ != 0) {
    ucontext_t *const context = static_cast<ucontext_t *>(native_context_);
    registers.eax = context->uc_mcontext.gregs[REG_EAX];
    registers.ebx = context->uc_mcontext.gregs[REG_EBX];
    registers.ecx = context->uc_mcontext.gregs[REG_ECX];
    registers.edx = context->uc_mcontext.gregs[REG_EDX];
    registers.esi = context->uc_mcontext.gregs[REG_ESI];
    registers.edi = context->uc_mcontext.gregs[REG_EDI];
    registers.ebp = context->uc_mcontext.gregs[REG_EBP];
    registers.esp = context->uc_mcontext.gregs[REG_ESP];
    registers.eip = context->uc_mcontext.gregs[REG_EIP];
    registers.eflags = context->uc_mcontext.gregs[REG_EFL];
  }
  return registers;
}

namespace {

int VisitModule(struct dl_phdr_info *info, size_t size, void *data) {
  std::vector<Module> *modules = reinterpret_cast<std::vector<Module> *>(data);

  const char *name = info->dlpi_name;
  if (modules->size() == 0) {
    // First visited entry is the main program.
    name = __progname;
  }

  uint32_t base_address = info->dlpi_addr;
  uint32_t total_size = 0;
  for (int i = 0; i < info->dlpi_phnum; i++) {
    total_size += info->dlpi_phdr[i].p_memsz;
  }

  Module module(name, base_address, total_size);
  modules->push_back(module);
  return 0;
}

} // namespace

void GetLoadedModules(std::vector<Module> &modules) {
  modules.clear();
  dl_iterate_phdr(VisitModule, reinterpret_cast<void *>(&modules));
}

std::string GetModuleName(void *address) {
  std::string filename;
  if (address != 0) {
    Dl_info info;
    dladdr(address, &info);
    filename.assign(info.dli_fname);
  }
  return filename;
}

namespace {

typedef void (*SignalHandler)(int signal, siginfo_t *info, void *context);

void SetSignalHandler(int signal,
                     SignalHandler handler,
                     struct sigaction *prev_action = 0) {
  struct sigaction action;
  sigemptyset(&action.sa_mask);
  action.sa_sigaction = handler;
  action.sa_flags = SA_SIGINFO;
  sigaction(signal, &action, prev_action);
}

void CallPreviousSignalHandler(int signal,
                               struct sigaction *prev_action = 0) {
  sigaction(signal, prev_action, 0);
  raise(signal);
}

CrashHandler crash_handler = 0;
struct sigaction prev_sigsegv_action;

static void HandleSIGSEGV(int signal, siginfo_t *info, void *context) {
  assert(signal == SIGSEGV || signal == SIGABRT);
  if (crash_handler != 0) {
    crash_handler(Context(context));
  }
  CallPreviousSignalHandler(signal, &prev_sigsegv_action);
}

} // namespace

void SetCrashHandler(CrashHandler handler) {
  crash_handler = handler;
  SetSignalHandler(SIGSEGV, HandleSIGSEGV, &prev_sigsegv_action);
  SetSignalHandler(SIGABRT, HandleSIGSEGV);
}

namespace {

InterruptHandler interrupt_handler;
struct sigaction prev_sigint_action;

void HandleSIGINT(int signal, siginfo_t *info, void *context) {
  assert(signal == SIGINT);
  if (interrupt_handler != 0) {
    interrupt_handler(Context(context));
  }
  CallPreviousSignalHandler(signal, &prev_sigint_action);
}

} // namespace

void SetInterruptHandler(InterruptHandler handler) {
  interrupt_handler = handler;
  SetSignalHandler(SIGINT, HandleSIGINT, &prev_sigint_action);
}

} // namespace os
