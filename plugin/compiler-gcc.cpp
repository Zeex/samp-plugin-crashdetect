// Copyright (c) 2012-2014 Zeex
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

#define FUNC "GetReturnAddress"
__asm__ (
  BEGIN_GLOBAL("_ZN8compiler16GetReturnAddressEPvi")
  "  movl 4(%esp), %eax\n"
  "  cmpl $0, %eax\n"
  "  jnz "FUNC"_init\n"
  "  movl %ebp, %eax\n"
  FUNC"_init:"
  "  movl 8(%esp), %ecx\n"
  "  movl $0, %edx\n"
  FUNC"_begin_loop:"
  "  cmpl $0, %ecx\n"
  "  jl "FUNC"_exit\n"
  "  movl 4(%eax), %edx\n"
  "  movl (%eax), %eax\n"
  "  decl %ecx\n"
  "  jmp "FUNC"_begin_loop\n"
  FUNC"_exit:"
  "  movl %edx, %eax\n"
  "  ret\n"
);
#undef FUNC

#define FUNC "GetStackFrame"
__asm__ (
  BEGIN_GLOBAL("_ZN8compiler13GetStackFrameEi")
  "  movl %ebp, %eax\n"
  "  movl 4(%esp), %ecx\n"
  FUNC"_begin_loop:"
  "  testl $0, %ecx\n"
  "  jz "FUNC"_exit\n"
  "  movl (%eax), %eax\n"
  "  decl %ecx\n"
  "  jmp "FUNC"_begin_loop\n"
  FUNC"_exit:"
  "  ret\n"
);
#undef FUNC

#define FUNC "GetStackTop"
__asm__ (
  BEGIN_GLOBAL("_ZN8compiler11GetStackTopEv")
  #ifdef _WIN32
  "  movl %fs:(0x04), %eax\n"
  #else
  "  movl $0xffffffff, %eax\n"
  #endif
  "  ret\n"
);
#undef FUNC

#define FUNC "GetStackBottom"
__asm__ (
  BEGIN_GLOBAL("_ZN8compiler14GetStackBottomEv")
  #ifdef _WIN32
  "  movl %fs:(0x08), %eax\n"
  #else
  "  xorl %eax, %eax\n"
  #endif
  "  ret\n"
);
#undef FUNC

#define FUNC "CallFunctionCdecl"
__asm__ (
  BEGIN_GLOBAL("_ZN8compiler17CallFunctionCdeclEPvPKvj")
  "  push %ebp\n"
  "  mov %esp, %ebp\n"
  "  push %esi\n"
  "  push %edi\n"
  "  push %ebx\n"
  "  mov 8(%ebp), %eax\n"
  "  mov 12(%ebp), %esi\n"
  "  mov 16(%ebp), %ebx\n"
  "  mov %ebx, %ecx\n"
  "  sub %ecx, %esp\n"
  "  mov %esp, %edi\n"
  "  rep movsb\n"
  "  call *%eax\n"
  "  add %ebx, %esp\n"
  "  pop %ebx\n"
  "  pop %edi\n"
  "  pop %esi\n"
  "  pop %ebp\n"
  "  ret\n"
);
#undef FUNC

#define FUNC "CallFunctionStdcall"
__asm__ (
  BEGIN_GLOBAL("_ZN8compiler19CallFunctionStdcallEPvPKvj")
  "  push %ebp\n"
  "  mov %esp, %ebp\n"
  "  push %esi\n"
  "  push %edi\n"
  "  push %ebx\n"
  "  mov 8(%ebp), %eax\n"
  "  mov 12(%ebp), %esi\n"
  "  mov 16(%ebp), %ebx\n"
  "  mov %ebx, %ecx\n"
  "  sub %ecx, %esp\n"
  "  mov %esp, %edi\n"
  "  rep movsb\n"
  "  call *%eax\n"
  "  pop %ebx\n"
  "  pop %edi\n"
  "  pop %esi\n"
  "  pop %ebp\n"
  "  ret\n"
);
#undef FUNC
