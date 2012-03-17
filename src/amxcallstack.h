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

#ifndef AMXCALLSTACK_H
#define AMXCALLSTACK_H

#include <string>
#include <deque>
#include <vector>

#include "amx/amx.h"
#include "amxdebuginfo.h"

class AMXStackFrame {
public:
	static const int kMaxString = 30;

	AMXStackFrame(AMX *amx, ucell frameAddr, const AMXDebugInfo &debugInfo = AMXDebugInfo());
	AMXStackFrame(AMX *amx, ucell frameAddr, ucell retAddr, const AMXDebugInfo &debugInfo = AMXDebugInfo());
	AMXStackFrame(AMX *amx, ucell frameAddr, ucell retAddr, ucell funAddr, const AMXDebugInfo &debugInfo = AMXDebugInfo());

	inline ucell GetFramAddress() const 
		{ return frameAddr_; }
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
	void Init(AMX *amx, const AMXDebugInfo &debugInfo);

	ucell frameAddr_;
	ucell retAddr_;
	ucell funAddr_;

	AMXDebugInfo::Symbol fun_;
	std::vector<AMXDebugInfo::Symbol> args_;

	std::string string_;
};

class AMXCallStack {
public:
	AMXCallStack(AMX *amx, const AMXDebugInfo &debugInfo, ucell topFrame = 0);

	std::deque<AMXStackFrame> GetFrames() const { return frames_; }

private:	
	std::deque<AMXStackFrame> frames_;
};

#endif // !AMXCALLSTACK_H
