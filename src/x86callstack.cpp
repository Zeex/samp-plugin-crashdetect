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

#include "x86callstack.h"

X86StackFrame::X86StackFrame(void *frmAddr, void *retAddr) 
	: frmAddr_(frmAddr), retAddr_(retAddr)
{

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
	void *frmAddr;
	void *retAddr;

	#if defined _MSC_VER
		__asm mov dword ptr [frmAddr], ebp
	#elif defined __GNUC__
		__asm__ __volatile__(
			"movl %%ebp, %0;" : "=r"(frmAddr) : : );
	#endif	

	do {
		retAddr = GetReturnAddress(frmAddr);
		if (retAddr == 0) {
			break;
		}
		frmAddr = GetNextFrame(frmAddr);
		frames_.push_back(X86StackFrame(frmAddr, retAddr));
	} while (true);
}