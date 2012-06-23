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
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

#include "compiler.h"
#include "os.h"
#include "x86stacktrace.h"

static const int kMaxSymbolNameLength = 256;

X86StackFrame::X86StackFrame(void *frmAddr, void *retAddr, const std::string &name)
	: frmAddr_(frmAddr), retAddr_(retAddr), name_(name)
{
}

std::string X86StackFrame::GetString() const {
	std::stringstream stream;

	stream << std::hex << std::setw(8) << std::setfill('0') 
		<< reinterpret_cast<long>(retAddr_) << std::dec;
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

X86StackTrace::X86StackTrace()
	: maxDepth_(std::numeric_limits<int>::max())
	, skipCount_(0)
	, topFrame_(0)
	, stackTop_(0)
	, stackBottom_(0)
{
}

std::deque<X86StackFrame> X86StackTrace::CollectFrames() const {
	std::deque<X86StackFrame> frames;

	void *frame = topFrame_ == 0
		? compiler::GetFrameAddress()
		: topFrame_;

	for (int i = 0; i < maxDepth_; i++) {
		if (frame == 0
			|| (frame >= stackTop_ && stackTop_ != 0)
			|| (frame < stackBottom_ && stackBottom_ != 0)) {
			break;
		}

		void *ret = GetReturnAddress(frame);
		if (ret == 0) {
			break;
		}

		frame = GetNextFrame(frame);

		if (i >= skipCount_) {
			frames.push_back(X86StackFrame(frame, ret, os::GetSymbolName(ret)));
		}
	}

	return frames;
}
