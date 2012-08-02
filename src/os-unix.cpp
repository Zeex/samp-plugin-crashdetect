// Copyright (c) 2011-2012, Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// // LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <csignal>
#include <cstring>
#include <string>
#include <vector>

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE 1 // for dladdr()
#endif
#include <dlfcn.h>

#include "os.h"

std::string os::GetModulePathFromAddr(void *address, std::size_t maxLength) {
	std::vector<char> name(maxLength + 1);
	if (address != 0) {
		Dl_info info;
		dladdr(address, &info);
		strncpy(&name[0], info.dli_fname, maxLength);
	}
	return std::string(&name[0]);
}

static os::ExceptionHandler exceptionHandler = 0;
static struct sigaction previousSIGSEGVAction;

static void HandleSIGSEGV(int sig, siginfo_t *info, void *context)
{
	if (::exceptionHandler != 0) {
		::exceptionHandler(context);
	}

	sigaction(sig, &::previousSIGSEGVAction, 0);
	raise(sig);
}

void os::SetExceptionHandler(ExceptionHandler handler) {
	::exceptionHandler = handler;

	struct sigaction action = {0};
	sigemptyset(&action.sa_mask);
	action.sa_sigaction = HandleSIGSEGV;
	action.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &action, &::previousSIGSEGVAction);
}

static os::InterruptHandler interruptHandler;
static struct sigaction previousSIGINTAction;

static void HandleSIGINT(int sig, siginfo_t *info, void *context) {
	if (::interruptHandler != 0) {
		::interruptHandler(context);
	}

	sigaction(sig, &::previousSIGINTAction, 0);
	raise(sig);
}

void os::SetInterruptHandler(InterruptHandler handler) {
	::interruptHandler = handler;

	struct sigaction action = {0};
	sigemptyset(&action.sa_mask);
	action.sa_sigaction = HandleSIGINT;
	action.sa_flags = SA_SIGINFO;

	sigaction(SIGINT, &action, &::previousSIGINTAction);
}
