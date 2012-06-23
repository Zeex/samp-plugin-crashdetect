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

StackTrace::StackTrace(int skip, int max) {
	void *trace[kMaxFrames];
	int traceLength = CaptureStackBackTrace(0, kMaxFrames, trace, NULL);

	HANDLE hProcess = GetCurrentProcess();
	SymInitialize(hProcess, NULL, TRUE);

	SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO*>(
			std::calloc(sizeof(*symbol) + kMaxSymbolNameLength, 1));
	symbol->SizeOfStruct = sizeof(*symbol);
	symbol->MaxNameLen = kMaxSymbolNameLength;

	for (int i = 0; i < traceLength && (i < max || max == 0); i++) {
		if (i >= skip) {
			SymFromAddr(hProcess, reinterpret_cast<DWORD64>(trace[i]), NULL, symbol);
			frames_.push_back(StackFrame(trace[i], symbol->Name));
			symbol->Name[0] = '\0';
		}
	}
}
