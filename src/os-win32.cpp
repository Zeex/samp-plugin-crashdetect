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

#include <cstddef>
#include <string>

#include <Windows.h>

#undef GetModulePath

std::string os::GetModulePath(void *address, std::size_t maxLength) {
	std::string name(maxLength, '\0');
	if (address != 0) {
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(address, &mbi, sizeof(mbi));
		GetModuleFileName((HMODULE)mbi.AllocationBase, const_cast<char*>(name.data()), maxLength);
	}	
	return name;
}

// The crash handler - it is set via SetCrashHandler()
static void (*crashHandler)() = 0;

// Previous exception filter
static LPTOP_LEVEL_EXCEPTION_FILTER previousExceptionFilter;

// Our exception filter
static LONG WINAPI ExceptionFilter(LPEXCEPTION_POINTERS exceptionInfo)
{
	if (::crashHandler != 0) {
		::crashHandler();
	}
	if (::previousExceptionFilter != 0) {
		return ::previousExceptionFilter(exceptionInfo);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

void os::SetCrashHandler(void (*handler)()) {
	::crashHandler = handler;
	if (handler != 0) {
		::previousExceptionFilter = SetUnhandledExceptionFilter(ExceptionFilter);
	} else {
		SetUnhandledExceptionFilter(::previousExceptionFilter);
	}
}

// The interrupt (Ctrl+C) handler - set via SetInterruptHandler
static void (*interruptHandler)();

// Console event handler
static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
	switch (dwCtrlType) {
	case CTRL_C_EVENT:
		if (::interruptHandler != 0) {
			::interruptHandler();
		}
	}
	return FALSE;
}

void os::SetInterruptHandler(void (*handler)()) {
	::interruptHandler = handler;
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}