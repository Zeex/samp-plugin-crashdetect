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

#include <Windows.h>

std::string os::GetModuleNameBySymbol(void *symbol) {
	char module[FILENAME_MAX] = "";

	if (symbol != 0) {
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(symbol, &mbi, sizeof(mbi));
		GetModuleFileName((HMODULE)mbi.AllocationBase, module, FILENAME_MAX);
	}
	
	return std::string(module);
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