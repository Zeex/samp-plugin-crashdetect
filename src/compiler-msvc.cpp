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

#include "compiler.h"

__declspec(naked) void *compiler::GetReturnAddress(void *frame, int depth) {
	__asm mov eax, dword ptr [esp + 4]
	__asm cmp eax, 0
	__asm jnz init
	__asm mov eax, ebp

init:
	__asm mov ecx, dword ptr [esp + 8]
	__asm mov edx, 0

iteration:
	__asm cmp ecx, 0
	__asm jl exit
	__asm mov edx, dword ptr [eax + 4]
	__asm mov eax, dword ptr [eax]
	__asm dec ecx
	__asm jmp iteration

exit:
	__asm mov eax, edx
	__asm ret
}

__declspec(naked) void *compiler::GetFrameAddress(int depth) {
	__asm mov eax, ebp
	__asm mov ecx, dword ptr [esp + 4]

iteration:
	__asm test ecx, 0
	__asm jz exit
	__asm mov eax, dword ptr [eax]
	__asm dec ecx
	__asm jmp iteration

exit:
	__asm ret
}

__declspec(naked) void *compiler::GetStackTop() {
	__asm mov eax, fs:[0x04]
	__asm ret
}

__declspec(naked) void *compiler::GetStackBottom() {
	__asm mov eax, fs:[0x08]
	__asm ret
}
