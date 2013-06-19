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

#define _GNU_SOURCE

#include <cassert>
#include <cstring>
#include <vector>

#include <dlfcn.h>
#include <signal.h>

#include "os.h"

os::Context::Context(ucontext_t *context)
  : os_context_(context)
{
  registers_[EAX] = context->uc_mcontext.gregs[REG_EAX];
  registers_[ECX] = context->uc_mcontext.gregs[REG_ECX];
  registers_[EDX] = context->uc_mcontext.gregs[REG_EDX];
  registers_[EBX] = context->uc_mcontext.gregs[REG_EBX];
  registers_[ESI] = context->uc_mcontext.gregs[REG_ESI];
  registers_[EDI] = context->uc_mcontext.gregs[REG_EDI];
  registers_[ESP] = context->uc_mcontext.gregs[REG_ESP];
  registers_[EBP] = context->uc_mcontext.gregs[REG_EBP];
  registers_[EIP] = context->uc_mcontext.gregs[REG_EIP];
  registers_[EFLAGS] = context->uc_mcontext.gregs[REG_EFL];
}

std::string os::GetModulePathFromAddr(void *address, std::size_t max_length) {
  std::vector<char> name(max_length + 1);
  if (address != 0) {
    Dl_info info;
    dladdr(address, &info);
    strncpy(&name[0], info.dli_fname, max_length);
  }
  return std::string(&name[0]);
}

static os::ExceptionHandler except_handler = 0;
static struct sigaction prev_sigsegv_action;
static __thread bool this_thread_handles_sigsegv;

static void HandleSIGSEGV(int sig, siginfo_t *info, void *ucontext) {
  if (!::this_thread_handles_sigsegv) {
    return;
  }

  if (::except_handler != 0) {
    os::Context context(reinterpret_cast<ucontext_t*>(ucontext));
    ::except_handler(context);
  }

  sigaction(sig, &::prev_sigsegv_action, 0);
  raise(sig);
}

void os::SetExceptionHandler(ExceptionHandler handler) {
  assert(::except_handler == 0 && "Only one thread may set exception handler");

  ::this_thread_handles_sigsegv = true;
  ::except_handler = handler;

  struct sigaction action;
  sigemptyset(&action.sa_mask);
  action.sa_sigaction = HandleSIGSEGV;
  action.sa_flags = SA_SIGINFO;

  sigaction(SIGSEGV, &action, &::prev_sigsegv_action);
}

static os::InterruptHandler interrupt_handler;
static struct sigaction prev_sigint_action;
static __thread bool this_thread_handles_sigint;

static void HandleSIGINT(int sig, siginfo_t *info, void *ucontext) {
  if (!::this_thread_handles_sigint) {
    return;
  }

  if (::interrupt_handler != 0) {
    os::Context context(reinterpret_cast<ucontext_t*>(ucontext));
    ::interrupt_handler(context);
  }

  sigaction(sig, &::prev_sigint_action, 0);
  raise(sig);
}

void os::SetInterruptHandler(InterruptHandler handler) {
  assert(::interrupt_handler == 0 && "Only one thread may set interrupt handler");

  ::this_thread_handles_sigint = true;
  ::interrupt_handler = handler;

  struct sigaction action;
  sigemptyset(&action.sa_mask);
  action.sa_sigaction = HandleSIGINT;
  action.sa_flags = SA_SIGINFO;

  sigaction(SIGINT, &action, &::prev_sigint_action);
}
