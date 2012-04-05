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

#include "x86callstack.h"

#ifdef WIN32
	#include <Windows.h>
	#include <DbgHelp.h>
#else
	#include <cxxabi.h>
	#include <execinfo.h>	
#endif

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

static const int kMaxSymbolNameLength = 256;

X86StackFrame::X86StackFrame(void *frmAddr, void *retAddr, const std::string &name) 
	: frmAddr_(frmAddr), retAddr_(retAddr), name_(name)
{
}

std::string X86StackFrame::GetString() const {
	std::stringstream stream;

	stream << std::hex << std::setw(8) << std::setfill('0') << retAddr_;
	if (!name_.empty()) {
	       stream << " in " << name_ << " ()";
	} else {
		stream << " in ?? ()";
	}
	
	return stream.str();
}

static inline void *GetReturnAddress(void *frmAddr) {
	return *reinterpret_cast<void**>(reinterpret_cast<char*>(frmAddr) + 4);
}

static inline void *GetNextFrame(void *frmAddr) {
	return *reinterpret_cast<void**>(frmAddr);
}

X86CallStack::X86CallStack()
	: frames_()
{
	#ifdef WIN32
		PVOID trace[62];
		WORD nframes = CaptureStackBackTrace(0, 62, trace, NULL);

		SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO*>(
				std::calloc(sizeof(*symbol) + kMaxSymbolNameLength, 1));

		symbol->SizeOfStruct = sizeof(*symbol);
		symbol->MaxNameLen = kMaxSymbolNameLength;

		SymInitialize(GetCurrentProcess(), NULL, TRUE);

		for (WORD i = 1; i < nframes; i++) {
			SymFromAddr(GetCurrentProcess(), reinterpret_cast<DWORD64>(trace[i]), NULL, symbol);
			frames_.push_back(X86StackFrame(0, trace[i], symbol->Name));
		}

		std::free(symbol);
	#else
		void *trace[100];

		std::size_t size = backtrace(trace, 100);
		char **strings = backtrace_symbols(trace, size);

		for (std::size_t i = 1; i < size; i++) {
			std::string string(strings[i]);

			std::string::size_type lp = string.find('(');
			std::string::size_type rp = string.find_first_of(")+-");

			std::string name;
			if (lp != std::string::npos && rp != std::string::npos) {
				name.assign(string.begin() + lp + 1, string.begin() + rp);
			}

			if (!name.empty()) {
				char *demangled_name = abi::__cxa_demangle(name.c_str(), 0, 0, 0);
				if (demangled_name != 0) {
					name.assign(demangled_name);
				}
			}

			frames_.push_back(X86StackFrame(0, trace[i], name));
		}

		std::free(strings);
	#endif
}