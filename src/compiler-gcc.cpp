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

#ifdef _WIN32
	#define SYMBOL(x) "_"x
#else
	#define SYMBOL(x) x
#endif

#define GLOBAL(x) ".globl "SYMBOL(x)";"

#define BEGIN_GLOBAL(x) \
	GLOBAL(x) \
	x":\n"

#define FUNC "GetReturnAddress"
__asm__ (
	BEGIN_GLOBAL("_ZN8compiler16GetReturnAddressEPvi")
	"	movl 4(%esp), %eax;"
	"	cmpl $0, %eax;"
	"	jnz "FUNC"_init;"
	"	movl %ebp, %eax;"
	FUNC"_init:"
	"	movl 8(%esp), %ecx;"
	"	movl $0, %edx;"
	FUNC"_loop:"
	"	cmpl $0, %ecx;"
	"	jl "FUNC"_exit;"
	"	movl 4(%eax), %edx;"
	"	movl (%eax), %eax;"
	"	decl %ecx;"
	"	jmp "FUNC"_loop;"
	FUNC"_exit:"
	"	movl %edx, %eax;"
	"	ret;"
);
#undef FUNC

#define FUNC "GetFrameAddress"
__asm__ (
	BEGIN_GLOBAL("_ZN8compiler15GetFrameAddressEi")
	"	movl %ebp, %eax;"
	"	movl 4(%esp), %ecx;"
	FUNC"_loop:"
	"	testl $0, %ecx;"
	"	jz "FUNC"_exit;"
	"	movl (%eax), %eax;"
	"	decl %ecx;"
	"	jmp "FUNC"_loop;"
	FUNC"_exit:"
	"	ret;"
);
#undef FUNC

#define FUNC "GetStackTop"
__asm__ (
	BEGIN_GLOBAL("_ZN8compiler11GetStackTopEv")
	#ifdef _WIN32
	"	movl %fs:(0x04), %eax;"
	#else
	"	xorl %eax, %eax;"
	#endif
	"	ret;"
);
#undef FUNC

#define FUNC "GetStackBottom"
__asm__ (
	BEGIN_GLOBAL("_ZN8compiler14GetStackBottomEv")
	#ifdef _WIN32
	"	movl %fs:(0x08), %eax;"
	#else
	"	xorl %eax, %eax;"
	#endif
	"	ret;"
);
#undef FUNC

#define FUNC "CallVariadicFunction"
__asm__ (
	BEGIN_GLOBAL("_ZN8compiler20CallVariadicFunctionEPvPKPKvi")
	"	movl 4(%esp), %eax;"
	"	movl 8(%esp), %edx;"
	"	movl 12(%esp), %ecx;"
	"	pushl %edi;"
	"	movl %ecx, %edi;"
	"	sal $2, %edi;"
	"	pushl %esi;"
	FUNC"_loop:"
	"	cmpl $0, %ecx;"
	"	jle "FUNC"_end_loop;"
	"	dec %ecx;"
	"	movl (%edx, %ecx, 4), %esi;"
	"	pushl %esi;"
	"	jmp "FUNC"_loop;"
	FUNC"_end_loop:"
	"	call *%eax;"
	"	addl %edi, %esp;"
	"	popl %esi;"
	"	popl %edi;"
	"	ret;"
);
#undef FUNC
