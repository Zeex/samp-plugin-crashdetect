// Copyright (c) 2011 Zeex
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "os.h"

#include <csignal>
#include <cstdio>
#include <cstring>

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE 1 // for dladdr()
#endif
#include <dlfcn.h> 

std::string os::GetModuleNameBySymbol(void *symbol) {
	char module[FILENAME_MAX] = "";

	if (symbol != 0) {
		Dl_info info;
		dladdr(symbol, &info);
		strcpy(module, info.dli_fname);
	}
	
	return std::string(module);
}

// The crash handler - it is set via SetCrashHandler()
static void (*crashHandler)() = 0;

// Previous SIGSEGV handler
static void (*previousSIGSEGVHandler)(int);

static void HandleSIGSEGV(int sig)
{
	if (::crashHandler != 0) {
		::crashHandler();
	}
	signal(sig, SIG_DFL);
}

void os::SetCrashHandler(void (*handler)()) {
	::crashHandler = handler;
	if (handler != 0) {
		::previousSIGSEGVHandler = signal(SIGSEGV, HandleSIGSEGV);
	} else {
		signal(SIGSEGV, ::previousSIGSEGVHandler);
	}
}

// The interrupt (Ctrl+C) handler - set via SetInterruptHandler
static void (*interruptHandler)();

// Previous SIGINT handler
static void (*previousSIGINTHandler)(int);

// Out SIGINT handler
static void HandleSIGINT(int sig) {
	if (::interruptHandler != 0) {
		::interruptHandler();
	}
	signal(sig, ::previousSIGINTHandler);
	raise(sig);
}

void os::SetInterruptHandler(void (*handler)()) {
	::interruptHandler = handler;
	::previousSIGINTHandler = signal(SIGINT, HandleSIGINT);
}
