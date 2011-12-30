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

#include <iosfwd>
#include <string>
#include <vector>

#include "amx/amx.h"
#include "amxdebuginfo.h"

class AMXStackFrame;

class AMXCallStack {
public:
	AMXCallStack(AMX *amx, const AMXDebugInfo &debugInfo);

	// Get list of stack frames
	std::vector<AMXStackFrame> GetFrames() const { return frames_; }

private:
	std::vector<AMXStackFrame> frames_;
};

class AMXStackFrame {
public:
	AMXStackFrame(AMX *amx, ucell frameAddress, 
			const AMXDebugInfo &debugInfo = AMXDebugInfo());
	AMXStackFrame(AMX *amx, ucell frameAddress, ucell callAddress, ucell functionAddress, 
			const AMXDebugInfo &debugInfo = AMXDebugInfo());

	// Returns true if the called function is public.
	inline bool IsPublic() const { return isPublic_; }

	// Always available.
	inline cell GetFrameAddress()    const { return frameAddress_; }
	inline cell GetCallAddress()     const { return callAddress_; }
	inline cell GetFunctionAddress() const { return functionAddress_; }

	// These require debug info (except GetFunctionName() which always works with publics).
	inline std::string GetSourceFileName()    const { return fileName_; }
	inline int32_t     GetLineNumber()        const { return lineNumber_; }
	inline std::string GetFunctionName()      const { return functionName_; } 
	inline std::string GetFunctionResultTag() const { return functionResultTag_; }
	inline std::string GetFunctionPrototype() const { return functionPrototype_; }

	inline std::vector<AMXDebugInfo::Symbol> GetArguments() const { return arguments_; }

	// Check if an address belongs to AMX stack.
	static inline bool IsOnStack(cell address, AMX *amx);

	// Returns true if construction succeeded.
	inline bool OK() const { return callAddress_ != 0; }
	inline operator bool() const { return OK(); }

private:
	// Contains the code common to both constructors.
	void Init(AMX *amx, const AMXDebugInfo &debugInfo);

	// Whether it's a public function.
	bool isPublic_;

	// The address of this stack frame.
	ucell frameAddress_;

	// Address of the CALL instruction (relative to COD).
	ucell callAddress_;

	// Address of the function itself.
	ucell functionAddress_;

	// A name of the source file from which the funciton is called.
	std::string fileName_;

	// A 1-based number of the line at which the function is called.
	uint32_t lineNumber_;

	std::string functionName_;
	std::string functionResultTag_;
	std::string functionPrototype_;

	// Formal parameters (looked up in symbol table).
	std::vector<AMXDebugInfo::Symbol> arguments_;
};

// static
inline bool AMXStackFrame::IsOnStack(cell address, AMX *amx) {
	return (address >= amx->stk && address <= amx->stp);
}

#endif // !AMXCALLSTACK_H
