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

#include <cstdlib>

#include <Windows.h>
#include <DbgHelp.h>

#include "stacktrace.h"

// MSDN says that CaptureStackBackTrace() can't handle more than 62 frames on
// Windows Server 2003 and Windows XP.
static const int kMaxFrames = 62;

class DbgHelp {
public:
	DbgHelp(HANDLE process = NULL);
	~DbgHelp();

	bool HaveSymInitialize() const {
		return SymInitialize_ != NULL;
	}
	BOOL SymInitialize(PCSTR UserSearchPath, BOOL fInvadeProcess) const {
		return SymInitialize_(process_, UserSearchPath, fInvadeProcess);
	}

	bool HaveSymFromAddr() const {
		return SymFromAddr_ != NULL;
	}
	BOOL SymFromAddr(DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol) const {
		return SymFromAddr_(process_, Address, Displacement, Symbol);
	}

	bool HaveSymCleanup() const {
		return SymCleanup_ != NULL;
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

private:
	typedef BOOL (WINAPI *SymInitializeType)(HANDLE, PCSTR, BOOL);
	SymInitializeType SymInitialize_;

	typedef BOOL (WINAPI *SymFromAddrType)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
	SymFromAddrType SymFromAddr_;

	typedef BOOL (WINAPI *SymCleanupType)(HANDLE);
	SymCleanupType SymCleanup_;

	HMODULE module_;
	HANDLE process_;
	BOOL initialized_;
};

DbgHelp::DbgHelp(HANDLE process)
	: module_(LoadLibrary("DbgHelp.dll"))
	, process_(process)
	, initialized_(FALSE)
{
	if (process_ == NULL) {
		process_ = GetCurrentProcess();
	}

	if (module_ != NULL) {
		SymInitialize_ = (SymInitializeType)GetProcAddress(module_, "SymInitialize");
		SymFromAddr_ = (SymFromAddrType)GetProcAddress(module_, "SymFromAddr");
		SymCleanup_ = (SymCleanupType)GetProcAddress(module_, "SymCleanup");
	}

	if (SymInitialize_ != NULL) {
		initialized_ = SymInitialize_(process_, NULL, TRUE);
	}
}

DbgHelp::~DbgHelp() {
	if (module_ != NULL) {
		if (initialized_ && SymCleanup_ != NULL) {
			SymCleanup_(process_);
		}
		FreeLibrary(module_);
	}
}

StackTrace::StackTrace(int skip, int max) {
	void *trace[kMaxFrames];
	int traceLength = CaptureStackBackTrace(0, kMaxFrames, trace, NULL);

	SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO*>(
			std::calloc(sizeof(*symbol) + kMaxSymbolNameLength, 1));
	symbol->SizeOfStruct = sizeof(*symbol);
	symbol->MaxNameLen = kMaxSymbolNameLength;

	DbgHelp dbghelp(GetCurrentProcess());

	for (int i = 0; i < traceLength && (i < max || max == 0); i++) {
		if (i >= skip) {
			CHAR *name = "";
			if (dbghelp.HaveSymFromAddr()) {
				if (dbghelp.SymFromAddr(reinterpret_cast<DWORD64>(trace[i]), NULL, symbol)) {
					name = symbol->Name;
				}
			}
			frames_.push_back(StackFrame(trace[i], name));
		}
	}
}
