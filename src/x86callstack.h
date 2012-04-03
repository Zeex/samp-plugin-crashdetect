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

#ifndef X86CALLSTACK_H
#define X86CALLSTACK_H

#include <vector>

class X86StackFrame {
public:
	X86StackFrame(void *frmAddr, void *retAddr_);

private:
	void *frmAddr_;
	void *retAddr_;
};

class X86CallStack {
public:
	X86CallStack();

	inline std::vector<X86StackFrame> GetFrames() const {
		return frames_;
	}

private:
	std::vector<X86StackFrame> frames_;
};

#endif // !X86CALLSTACK_H
