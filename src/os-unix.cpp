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

#include "os.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <dirent.h>
#include <fnmatch.h>

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE 1 // for dladdr()
#endif
#include <dlfcn.h> 

const char os::kDirSepChar = '/';

std::string os::GetModulePath(void *address, std::size_t maxLength) {
	std::vector<char> name(maxLength + 1);
	if (address != 0) {
		Dl_info info;
		dladdr(address, &info);
		strncpy(&name[0], info.dli_fname, maxLength);
	}	
	return std::string(&name[0]);
}

// The exception handler - it is set via SetExceptionHandler()
static os::ExceptionHandler exceptionHandler = 0;

// Previous SIGSEGV handler
static void (*previousSIGSEGVHandler)(int);

static void HandleSIGSEGV(int sig)
{
	if (::exceptionHandler != 0) {
		::exceptionHandler();
	}
	signal(sig, SIG_DFL);
}

void os::SetExceptionHandler(ExceptionHandler handler) {::exceptionHandler = handler;
	if (handler != 0) {
		::previousSIGSEGVHandler = signal(SIGSEGV, HandleSIGSEGV);
	} else {
		signal(SIGSEGV, ::previousSIGSEGVHandler);
	}
}

// The interrupt (Ctrl+C) handler - set via SetInterruptHandler
static os::InterruptHandler interruptHandler;

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

void os::SetInterruptHandler(InterruptHandler handler) {
	::interruptHandler = handler;
	::previousSIGINTHandler = signal(SIGINT, HandleSIGINT);
}

void os::ListDirectoryFiles(const std::string &directory, const std::string &pattern,
		bool (*callback)(const char *, void *), void *userData) 
{
	DIR *dp;
	if ((dp = opendir(directory.c_str())) != 0) {
		struct dirent *dirp;
		while ((dirp = readdir(dp)) != 0) {
			if (!fnmatch(pattern.c_str(), dirp->d_name, FNM_CASEFOLD | FNM_NOESCAPE | FNM_PERIOD)) {
				callback(dirp->d_name, userData);
			}
		}
		closedir(dp);
	}
}
