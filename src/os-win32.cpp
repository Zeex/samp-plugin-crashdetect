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
#include <cstdlib>
#include <string>
#include <vector>

#include <Windows.h>
#include <DbgHelp.h>
#include <sys/types.h>

#if defined __GNUC__
	#include <cxxabi.h>
#endif

const char os::kDirSepChar = '\\';

#undef GetModulePath

std::string os::GetModulePath(void *address, std::size_t maxLength) {
	std::vector<char> name(maxLength + 1);
	if (address != 0) {
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(address, &mbi, sizeof(mbi));
		GetModuleFileName((HMODULE)mbi.AllocationBase, &name[0], maxLength);
	}
	return std::string(&name[0]);
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

class DbgHelp {
public:
	DbgHelp();
	~DbgHelp();

	BOOL SymInitialize(HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess) const;
	BOOL SymFromAddr(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol) const;

	inline bool OK() const {
		return module_ != 0;
	}
	inline operator bool() const {
		return OK();
	}

private:
	typedef BOOL (WINAPI *SymInitializeType)(HANDLE, PCSTR, BOOL);
	SymInitializeType SymInitialize_;

	typedef BOOL (WINAPI *SymFromAddrType)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
	SymFromAddrType SymFromAddr_;

	HMODULE module_;
};

DbgHelp::DbgHelp()
	: module_(LoadLibrary("dbghelp.dll"))
{
	SymInitialize_ = (SymInitializeType)GetProcAddress(module_, "SymInitialize");
	SymFromAddr_ = (SymFromAddrType)GetProcAddress(module_, "SymFromAddr");
}

DbgHelp::~DbgHelp() {
	if (module_ != NULL) {
		FreeLibrary(module_);
	}
}

BOOL DbgHelp::SymInitialize(HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess) const {
	return SymInitialize_(hProcess, UserSearchPath, fInvadeProcess);
}

BOOL DbgHelp::SymFromAddr(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol) const {
	return SymFromAddr_(hProcess, Address, Displacement, Symbol);
}

std::string os::GetSymbolName(void *address, std::size_t maxLength) {
	std::string name;

	DbgHelp dbgHelpDll;
	if (dbgHelpDll) {
		SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO*>(
				std::calloc(sizeof(*symbol) + maxLength, 1));
		symbol->SizeOfStruct = sizeof(*symbol);
		symbol->MaxNameLen = maxLength;

		HANDLE process = GetCurrentProcess();

		dbgHelpDll.SymInitialize(process, NULL, TRUE);
		dbgHelpDll.SymFromAddr(process, reinterpret_cast<DWORD64>(address), NULL, symbol);
		name.assign(symbol->Name);

		#if defined __GNUC__
			if (!name.empty()) {
				char *demangled_name = abi::__cxa_demangle(("_" + name).c_str(), 0, 0, 0);
				if (demangled_name != 0) {
					name.assign(demangled_name);
				}
			}
		#endif

		std::free(symbol);
	}

	return name;
}

void os::ListDirectoryFiles(const std::string &directory, const std::string &pattern,
		bool (*callback)(const char *, void *), void *userData) 
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFindFile = FindFirstFile((directory + "\\" + pattern).c_str(), &findFileData);
	if (hFindFile != INVALID_HANDLE_VALUE) {
		do {
			if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				callback(findFileData.cFileName, userData);
			}
		} while (FindNextFile(hFindFile, &findFileData) != 0);
		FindClose(hFindFile);
	}
}
