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

#ifndef AMXCALLSTACK_H
#define AMXCALLSTACK_H

#include <string>
#include <deque>
#include <vector>

#include <amx/amx.h>

#include "amxdebuginfo.h"

class AMXStackFrame {
public:
	static const int kMaxString = 30;

	AMXStackFrame(AMX *amx, ucell frmAddr, const AMXDebugInfo &debugInfo = AMXDebugInfo());
	AMXStackFrame(AMX *amx, ucell frmAddr, ucell retAddr, const AMXDebugInfo &debugInfo = AMXDebugInfo());
	AMXStackFrame(AMX *amx, ucell frmAddr, ucell retAddr, ucell funAddr, const AMXDebugInfo &debugInfo = AMXDebugInfo());

	inline ucell GetFrameAddress() const 
		{ return frmAddr_; }
	inline ucell GetReturnAddress() const 
		{ return retAddr_; }
	inline ucell GetFunctionAddress() const
		{ return funAddr_; }
	inline std::string GetString() const 
		{ return string_; }
	inline AMXDebugInfo::Symbol GetFunction() const
		{ return fun_; }
	inline std::vector<AMXDebugInfo::Symbol> GetArguments() const
		{ return args_; }

private:
	void Init(AMX *amx, ucell frmAddr, ucell retAddr, ucell funAddr, const AMXDebugInfo &debugInfo);

	ucell frmAddr_;
	ucell retAddr_;
	ucell funAddr_;

	AMXDebugInfo::Symbol fun_;
	std::vector<AMXDebugInfo::Symbol> args_;

	std::string string_;
};

class AMXStackTrace {
public:
	AMXStackTrace(AMX *amx, const AMXDebugInfo &debugInfo, ucell topFrame = 0);

	std::deque<AMXStackFrame> GetFrames() const { return frames_; }

private:	
	std::deque<AMXStackFrame> frames_;
};

#endif // !AMXCALLSTACK_H
