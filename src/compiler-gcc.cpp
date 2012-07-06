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

#include "compiler.h"

__asm__ __volatile__(
#ifdef __MINGW32__
".globl __ZN8compiler16GetReturnAddressEPvi;"
"__ZN8compiler16GetReturnAddressEPvi:"
#else
"_ZN8compiler16GetReturnAddressEPvi:"
".globl _ZN8compiler16GetReturnAddressEPvi;"
#endif

"	movl 4(%esp), %eax;"
"	cmpl $0, %eax;"
"	jnz GetReturnAddress_init;"
"	movl %ebp, %eax;"

"GetReturnAddress_init:"
"	movl 8(%esp), %ecx;"
"	movl $0, %edx;"

"GetReturnAddress_loop:"
"	cmpl $0, %ecx;"
"	jl GetReturnAddress_exit;"
"	movl 4(%eax), %edx;"
"	movl (%eax), %eax;"
"	decl %ecx;"
"	jmp GetReturnAddress_loop;"

"GetReturnAddress_exit:"
"	movl %edx, %eax;"
"	ret;"
);

__asm__ __volatile__ (
#ifdef __MINGW32__
".globl __ZN8compiler15GetFrameAddressEi;"
"__ZN8compiler15GetFrameAddressEi:"
#else
".globl _ZN8compiler15GetFrameAddressEi;"
"_ZN8compiler15GetFrameAddressEi:"
#endif

"	movl %ebp, %eax;"
"	movl 4(%esp), %ecx;"

"GetFrameAddress_loop:"
"	testl $0, %ecx;"
"	jz GetFrameAddress_exit;"
"	movl (%eax), %eax;"
"	decl %ecx;"
"	jmp GetFrameAddress_loop;"

"GetFrameAddress_exit:"
"	ret;"
);

__asm__ __volatile__ (
#ifdef __MINGW32__
".globl __ZN8compiler11GetStackTopEv;"
"__ZN8compiler11GetStackTopEv:"

"	movl %fs:(0x04), %eax;"
"	ret;"
#else
".globl _ZN8compiler11GetStackTopEv;"
"_ZN8compiler11GetStackTopEv:"

"	xorl %eax, %eax;"
"	ret;"
#endif
);

__asm__ __volatile__ (
#ifdef __MINGW32__
".globl __ZN8compiler14GetStackBottomEv;"
"__ZN8compiler14GetStackBottomEv:"

"	movl %fs:(0x08), %eax;"
"	ret;"
#else
".globl _ZN8compiler14GetStackBottomEv;"
"_ZN8compiler14GetStackBottomEv:"

"	xorl %eax, %eax;"
"	ret;"
#endif
);
