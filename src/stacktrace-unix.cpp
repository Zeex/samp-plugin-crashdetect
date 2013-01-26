// Copyright (c) 2012 Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <string>

#include <execinfo.h>

#include "stacktrace.h"
#include "stacktrace-manual.h"

static const int kMaxFrames = 100;

static std::string GetSymbolName(const std::string &symbol) {
	std::string name;

	if (!symbol.empty()) {
		std::string::size_type lp = symbol.find('(');
		std::string::size_type rp = symbol.find_first_of(")+-", lp);

		if (lp != std::string::npos && rp != std::string::npos) {
			name = symbol.substr(lp + 1, rp - lp - 1);
		}
	}

	return name;
}

StackTrace::StackTrace(void *context) {
	#ifdef HAVE_BACKTRACE
		void *trace[kMaxFrames];
		int trace_length = backtrace(trace, kMaxFrames);

		#ifdef HAVE_BACKTRACE_SYMBOLS
			char **symbols = backtrace_symbols(trace, trace_length);
		#endif

		for (int i = 0; i < trace_length; i++) {
			#ifdef HAVE_BACKTRACE_SYMBOLS
				if (symbols[i] != 0) {
					std::string name = GetSymbolName(symbols[i]);
					frames_.push_back(StackFrame(trace[i], name));
				} else {
					frames_.push_back(StackFrame(trace[i]));
				}
			#else
				frames_.push_back(StackFrame(trace[i]));
			#endif
		}
	#else
		frames_ = StackTraceManual().GetFrames();
	#endif
}
