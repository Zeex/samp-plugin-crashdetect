// Copyright (c) 2013 Zeex
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

#include <windows.h>
#include <process.h>

#include "thread.h"

class ThreadSystemInfo {
 public:
  ThreadSystemInfo(uintptr_t handle)
   : handle_(handle),
     finished_(false)
  {
  }

  uintptr_t handle() const { return handle_; }
  void set_handle(uintptr_t handle) { handle_ = handle; }

  bool is_finished() const { return finished_; }
  void set_finished(bool finished) { finished_ = finished; }

 private:
  uintptr_t handle_;
  bool finished_;
};

class ThreadRunInfo {
 public:
  ThreadRunInfo(Thread *thread, void *args)
   : thread_(thread),
     args_(args)
  {
  }

  Thread *thread() { return thread_; }
  void *args() { return args_; }

 private:
  Thread *thread_;
  void *args_;
};

void __cdecl RunThread(void *args) {
  ThreadRunInfo *run_info = (ThreadRunInfo *)args;
  Thread *thread = run_info->thread();
  thread->Start(run_info->args());
  thread->Finish();
  delete run_info;
}

Thread::Thread()
 : func_(0),
   info_(new ThreadSystemInfo(0))
{
}

Thread::Thread(ThreadRoutine func)
 : func_(func),
   info_(new ThreadSystemInfo(0))
{
}

Thread::~Thread() {
  delete info_;
}

void Thread::Run(void *args) {
  ThreadRunInfo *run_info = new ThreadRunInfo(this, args);
  info_->set_finished(false);
  info_->set_handle(_beginthread(&RunThread, 0, (void *)run_info));
}

void Thread::Start(void *args) {
  func_(args);
}

void Thread::Join() {
  while (!info_->is_finished()) {
    Sleep(1);
  }
}

void Thread::Finish() {
  info_->set_finished(true);
}
