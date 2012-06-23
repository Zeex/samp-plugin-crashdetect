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

#ifndef X86CALLSTACK_H
#define X86CALLSTACK_H

#include <deque>
#include <string>

class X86StackFrame {
public:
	X86StackFrame(void *frmAddr, void *retAddr, const std::string &name = std::string());

	inline void *GetFrameAddress() const {
		return frmAddr_;
	}

	inline void *GetReturnAddress() const {
		return retAddr_;
	}

	// Get caller name if possible. An empty string can be returned.
	inline std::string GetFunctionName() const {
		return name_;
	}

	std::string GetString() const;

private:
	void *frmAddr_;
	void *retAddr_;
	std::string name_;
};

class X86StackTrace {
public:
	X86StackTrace();

	// Set the maximum number of trace items.
	inline void SetMaxDepth(int number) {
		maxDepth_ = number;
	}

	// How much frames to skip in beginning.
	inline void SetSkipCount(int number) {
		skipCount_ = number;
	}

	// Set the top most frame at which stack trace will start.
	inline void SetTopFrame(void *ptr) {
		topFrame_ = ptr;
	}

	// Set stack upper bound.
	inline void SetStackTop(void *ptr) {
		stackTop_ = ptr;
	}

	// Set stack lower bound.
	inline void SetStackBottom(void *ptr) {
		stackBottom_ = ptr;
	}

	std::deque<X86StackFrame> CollectFrames() const;

private:
	int maxDepth_;
	int skipCount_;
	void *topFrame_;
	void *stackTop_;
	void *stackBottom_;
};

#endif // !X86CALLSTACK_H
