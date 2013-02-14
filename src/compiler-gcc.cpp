// Copyright (c) 2012-2013 Zeex
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

#ifdef _WIN32
	#define SYMBOL(x) "_"x
#else
	#define SYMBOL(x) x
#endif

#define GLOBAL(x) ".globl "SYMBOL(x)"\n"

#define BEGIN_GLOBAL(x) \
	GLOBAL(x) \
	x":\n"

#define FUNC "GetReturnAddr"
__asm__ (
	BEGIN_GLOBAL("_ZN8compiler10GetRetAddrEPvi")
	"	movl 4(%esp), %eax\n"
	"	cmpl $0, %eax\n"
	"	jnz "FUNC"_init\n"
	"	movl %ebp, %eax\n"
	FUNC"_init:"
	"	movl 8(%esp), %ecx\n"
	"	movl $0, %edx\n"
	FUNC"_begin_loop:"
	"	cmpl $0, %ecx\n"
	"	jl "FUNC"_exit\n"
	"	movl 4(%eax), %edx\n"
	"	movl (%eax), %eax\n"
	"	decl %ecx\n"
	"	jmp "FUNC"_begin_loop\n"
	FUNC"_exit:"
	"	movl %edx, %eax\n"
	"	ret\n"
);
#undef FUNC

#define FUNC "GetStackFrame"
__asm__ (
	BEGIN_GLOBAL("_ZN8compiler13GetStackFrameEi")
	"	movl %ebp, %eax\n"
	"	movl 4(%esp), %ecx\n"
	FUNC"_begin_loop:"
	"	testl $0, %ecx\n"
	"	jz "FUNC"_exit\n"
	"	movl (%eax), %eax\n"
	"	decl %ecx\n"
	"	jmp "FUNC"_begin_loop\n"
	FUNC"_exit:"
	"	ret\n"
);
#undef FUNC

#define FUNC "GetStackTop"
__asm__ (
	BEGIN_GLOBAL("_ZN8compiler11GetStackTopEv")
	#ifdef _WIN32
	"	movl %fs:(0x04), %eax\n"
	#else
	"	movl $0xffffffff, %eax\n"
	#endif
	"	ret\n"
);
#undef FUNC

#define FUNC "GetStackBottom"
__asm__ (
	BEGIN_GLOBAL("_ZN8compiler14GetStackBottomEv")
	#ifdef _WIN32
	"	movl %fs:(0x08), %eax\n"
	#else
	"	xorl %eax, %eax\n"
	#endif
	"	ret\n"
);
#undef FUNC

#define FUNC "CallCdeclFunc"
__asm__ (
	BEGIN_GLOBAL("_ZN8compiler13CallCdeclFuncEPvPKPKvi")
	"	movl 4(%esp), %eax\n"
	"	movl 8(%esp), %edx\n"
	"	movl 12(%esp), %ecx\n"
	"	pushl %edi\n"
	"	movl %ecx, %edi\n"
	"	sal $2, %edi\n"
	"	pushl %esi\n"
	FUNC"_begin_loop:"
	"	cmpl $0, %ecx\n"
	"	jle "FUNC"_end_loop\n"
	"	dec %ecx\n"
	"	movl (%edx, %ecx, 4), %esi\n"
	"	pushl %esi\n"
	"	jmp "FUNC"_begin_loop\n"
	FUNC"_end_loop:"
	"	call *%eax\n"
	"	addl %edi, %esp\n"
	"	popl %esi\n"
	"	popl %edi\n"
	"	ret\n"
);
#undef FUNC

#define FUNC "CallStdcallFunc"
__asm__ (
	BEGIN_GLOBAL("_ZN8compiler15CallStdcallFuncEPvPKPKvi")
	"	movl 4(%esp), %eax\n"
	"	movl 8(%esp), %edx\n"
	"	movl 12(%esp), %ecx\n"
	"	pushl %esi\n"
	FUNC"_begin_loop:"
	"	cmpl $0, %ecx\n"
	"	jle "FUNC"_end_loop\n"
	"	dec %ecx\n"
	"	movl (%edx, %ecx, 4), %esi\n"
	"	pushl %esi\n"
	"	jmp "FUNC"_begin_loop\n"
	FUNC"_end_loop:"
	"	call *%eax\n"
	"	popl %esi\n"
	"	ret\n"
);
#undef FUNC
