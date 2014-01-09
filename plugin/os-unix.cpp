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
#include <cstring>
#include <vector>

#include <dlfcn.h>
#include <signal.h>

#include "os.h"

std::string os::GetModuleName(void *address) {
  std::string filename;
  if (address != 0) {
    Dl_info info;
    dladdr(address, &info);
    filename.assign(info.dli_fname);
  }
  return filename;
}

typedef void (*SignalHandler)(int signal, siginfo_t *info, void *context);

static void SetSignalHandler(int signal, SignalHandler handler,
                             struct sigaction *prev_action = 0) {
  struct sigaction action;
  sigemptyset(&action.sa_mask);
  action.sa_sigaction = handler;
  action.sa_flags = SA_SIGINFO;
  sigaction(signal, &action, prev_action);
}

static void CallPreviousSignalHandler(int signal,
                                      struct sigaction *prev_action = 0) {
  sigaction(signal, prev_action, 0);
  raise(signal);
}

static os::ExceptionHandler except_handler = 0;
static struct sigaction prev_sigsegv_action;

static void HandleSIGSEGV(int signal, siginfo_t *info, void *context) {
  assert(signal == SIGSEGV || signal == SIGABRT);
  if (::except_handler != 0) {
    ::except_handler(context);
  }
  CallPreviousSignalHandler(signal, &::prev_sigsegv_action);
}

void os::SetExceptionHandler(ExceptionHandler handler) {
  ::except_handler = handler;
  SetSignalHandler(SIGSEGV, HandleSIGSEGV, &::prev_sigsegv_action);
  SetSignalHandler(SIGABRT, HandleSIGSEGV);
}

static os::InterruptHandler interrupt_handler;
static struct sigaction prev_sigint_action;

static void HandleSIGINT(int signal, siginfo_t *info, void *context) {
  assert(signal == SIGINT);
  if (::interrupt_handler != 0) {
    ::interrupt_handler(context);
  }
  CallPreviousSignalHandler(signal, &::prev_sigint_action);
}

void os::SetInterruptHandler(InterruptHandler handler) {
  ::interrupt_handler = handler;
  SetSignalHandler(SIGINT, HandleSIGINT, &::prev_sigint_action);
}
