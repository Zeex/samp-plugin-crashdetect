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
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
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

#if defined GetModulePath
	#undef GetModulePath
#endif

std::string os::GetModulePath(void *address, std::size_t maxLength) {
	std::vector<char> name(maxLength + 1);
	if (address != 0) {
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(address, &mbi, sizeof(mbi));
		GetModuleFileName((HMODULE)mbi.AllocationBase, &name[0], maxLength);
	}
	return std::string(&name[0]);
}

// The exception handler - it is set via SetExceptionHandler()
static os::ExceptionHandler exceptionHandler = 0;

// Previous exception filter
static LPTOP_LEVEL_EXCEPTION_FILTER previousExceptionFilter;

// Our exception filter
static LONG WINAPI ExceptionFilter(LPEXCEPTION_POINTERS exceptionInfo) {
	if (::exceptionHandler != 0) {
		os::ExceptionContext ctx;
		ctx.SetEbp(reinterpret_cast<void*>(exceptionInfo->ContextRecord->Ebp));
		ctx.SetEsp(reinterpret_cast<void*>(exceptionInfo->ContextRecord->Esp));
		::exceptionHandler(&ctx);
	}
	if (::previousExceptionFilter != 0) {
		return ::previousExceptionFilter(exceptionInfo);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

void os::SetExceptionHandler(ExceptionHandler handler) {
	::exceptionHandler = handler;
	if (handler != 0) {
		::previousExceptionFilter = SetUnhandledExceptionFilter(ExceptionFilter);
	} else {
		SetUnhandledExceptionFilter(::previousExceptionFilter);
	}
}

// The interrupt (Ctrl+C) handler - set via SetInterruptHandler
static os::InterruptHandler interruptHandler;

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

void os::SetInterruptHandler(InterruptHandler handler) {
	::interruptHandler = handler;
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

class DbgHelp {
public:
	~DbgHelp();

	BOOL SymInitialize(PCSTR UserSearchPath, BOOL fInvadeProcess) const {
		return SymInitialize_(process_, UserSearchPath, fInvadeProcess);
	}
	BOOL SymFromAddr(DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol) const {
		return SymFromAddr_(process_, Address, Displacement, Symbol);
	}
	BOOL SymCleanup() const {
		return SymCleanup_(process_);
	}

	inline bool IsLoaded() const {
		return module_ != 0;
	}
	inline bool IsInitialized() const {
		return initialized_ != FALSE;
	}

	static DbgHelp &GetGlobal();

private:
	DbgHelp(bool initialie = true);

	typedef BOOL (WINAPI *SymInitializeType)(HANDLE, PCSTR, BOOL);
	SymInitializeType SymInitialize_;

	typedef BOOL (WINAPI *SymFromAddrType)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
	SymFromAddrType SymFromAddr_;

	typedef BOOL (WINAPI *SymCleanupType)(HANDLE);
	SymCleanupType SymCleanup_;
	
	BOOL initialized_;
	HANDLE process_;
	HMODULE module_;
};

DbgHelp::DbgHelp(bool initialize)
	: module_(LoadLibrary("dbghelp.dll"))
	, process_(GetCurrentProcess())
	, initialized_(FALSE)
{
	module_ = LoadLibrary("dbghelp.dll");

	SymInitialize_ = (SymInitializeType)GetProcAddress(module_, "SymInitialize");
	SymFromAddr_ = (SymFromAddrType)GetProcAddress(module_, "SymFromAddr");
	SymCleanup_ = (SymCleanupType)GetProcAddress(module_, "SymCleanup");

	if (initialize) {
		initialized_ = this->SymInitialize(NULL, TRUE);
	}
}

DbgHelp::~DbgHelp() {
	if (module_ != NULL) {
		if (initialized_) {
			this->SymCleanup();
		}
		FreeLibrary(module_);
	}
}

// static
DbgHelp &DbgHelp::GetGlobal() {
	static DbgHelp dll;
	return dll;
}

std::string os::GetSymbolName(void *address, std::size_t maxLength) {
	std::string name;

	DbgHelp &dbghelp = DbgHelp::GetGlobal();
	if (dbghelp.IsLoaded()) {
		SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO*>(
				std::calloc(sizeof(*symbol) + maxLength, 1));
		symbol->SizeOfStruct = sizeof(*symbol);
		symbol->MaxNameLen = maxLength;

		dbghelp.SymFromAddr(reinterpret_cast<DWORD64>(address), NULL, symbol);
		name.assign(symbol->Name);

		#if defined __GNUC__
			if (!name.empty()) {
				char *demangled_name = abi::__cxa_demangle(("_" + name).c_str(), 0, 0, 0);
				if (demangled_name != 0) {
					name.assign(demangled_name);
				}
				std::string::size_type end = name.find('(');
				if (end != std::string::npos) {
					name.erase(end);
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
