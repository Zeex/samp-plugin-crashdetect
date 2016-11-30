// Copyright (c) 2013-2015 Zeex
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

#ifndef AMXOPCODE_H
#define AMXOPCODE_H

#include <amx/amx.h>

enum AMXOpcode {
  AMX_OP_NONE,         AMX_OP_LOAD_PRI,     AMX_OP_LOAD_ALT,
  AMX_OP_LOAD_S_PRI,   AMX_OP_LOAD_S_ALT,   AMX_OP_LREF_PRI,
  AMX_OP_LREF_ALT,     AMX_OP_LREF_S_PRI,   AMX_OP_LREF_S_ALT,
  AMX_OP_LOAD_I,       AMX_OP_LODB_I,       AMX_OP_CONST_PRI,
  AMX_OP_CONST_ALT,    AMX_OP_ADDR_PRI,     AMX_OP_ADDR_ALT,
  AMX_OP_STOR_PRI,     AMX_OP_STOR_ALT,     AMX_OP_STOR_S_PRI,
  AMX_OP_STOR_S_ALT,   AMX_OP_SREF_PRI,     AMX_OP_SREF_ALT,
  AMX_OP_SREF_S_PRI,   AMX_OP_SREF_S_ALT,   AMX_OP_STOR_I,
  AMX_OP_STRB_I,       AMX_OP_LIDX,         AMX_OP_LIDX_B,
  AMX_OP_IDXADDR,      AMX_OP_IDXADDR_B,    AMX_OP_ALIGN_PRI,
  AMX_OP_ALIGN_ALT,    AMX_OP_LCTRL,        AMX_OP_SCTRL,
  AMX_OP_MOVE_PRI,     AMX_OP_MOVE_ALT,     AMX_OP_XCHG,
  AMX_OP_PUSH_PRI,     AMX_OP_PUSH_ALT,     AMX_OP_PUSH_R,
  AMX_OP_PUSH_C,       AMX_OP_PUSH,         AMX_OP_PUSH_S,
  AMX_OP_PAMX_OP_PRI,  AMX_OP_PAMX_OP_ALT,  AMX_OP_STACK,
  AMX_OP_HEAP,         AMX_OP_PROC,         AMX_OP_RET,
  AMX_OP_RETN,         AMX_OP_CALL,         AMX_OP_CALL_PRI,
  AMX_OP_JUMP,         AMX_OP_JREL,         AMX_OP_JZER,
  AMX_OP_JNZ,          AMX_OP_JEQ,          AMX_OP_JNEQ,
  AMX_OP_JLESS,        AMX_OP_JLEQ,         AMX_OP_JGRTR,
  AMX_OP_JGEQ,         AMX_OP_JSLESS,       AMX_OP_JSLEQ,
  AMX_OP_JSGRTR,       AMX_OP_JSGEQ,        AMX_OP_SHL,
  AMX_OP_SHR,          AMX_OP_SSHR,         AMX_OP_SHL_C_PRI,
  AMX_OP_SHL_C_ALT,    AMX_OP_SHR_C_PRI,    AMX_OP_SHR_C_ALT,
  AMX_OP_SMUL,         AMX_OP_SDIV,         AMX_OP_SDIV_ALT,
  AMX_OP_UMUL,         AMX_OP_UDIV,         AMX_OP_UDIV_ALT,
  AMX_OP_ADD,          AMX_OP_SUB,          AMX_OP_SUB_ALT,
  AMX_OP_AND,          AMX_OP_OR,           AMX_OP_XOR,
  AMX_OP_NOT,          AMX_OP_NEG,          AMX_OP_INVERT,
  AMX_OP_ADD_C,        AMX_OP_SMUL_C,       AMX_OP_ZERO_PRI,
  AMX_OP_ZERO_ALT,     AMX_OP_ZERO,         AMX_OP_ZERO_S,
  AMX_OP_SIGN_PRI,     AMX_OP_SIGN_ALT,     AMX_OP_EQ,
  AMX_OP_NEQ,          AMX_OP_LESS,         AMX_OP_LEQ,
  AMX_OP_GRTR,         AMX_OP_GEQ,          AMX_OP_SLESS,
  AMX_OP_SLEQ,         AMX_OP_SGRTR,        AMX_OP_SGEQ,
  AMX_OP_EQ_C_PRI,     AMX_OP_EQ_C_ALT,     AMX_OP_INC_PRI,
  AMX_OP_INC_ALT,      AMX_OP_INC,          AMX_OP_INC_S,
  AMX_OP_INC_I,        AMX_OP_DEC_PRI,      AMX_OP_DEC_ALT,
  AMX_OP_DEC,          AMX_OP_DEC_S,        AMX_OP_DEC_I,
  AMX_OP_MOVS,         AMX_OP_CMPS,         AMX_OP_FILL,
  AMX_OP_HALT,         AMX_OP_BOUNDS,       AMX_OP_SYSREQ_PRI,
  AMX_OP_SYSREQ_C,     AMX_OP_FILE,         AMX_OP_LINE,
  AMX_OP_SYMBOL,       AMX_OP_SRANGE,       AMX_OP_JUMP_PRI,
  AMX_OP_SWITCH,       AMX_OP_CASETBL,      AMX_OP_SWAP_PRI,
  AMX_OP_SWAP_ALT,     AMX_OP_PUSH_ADR,     AMX_OP_NOP,
  AMX_OP_SYSREQ_D,     AMX_OP_SYMTAG,       AMX_OP_BREAK,
  AMX_OP_LAST_
};

const int NUM_AMX_OPCODES = AMX_OP_LAST_;

cell RelocateAMXOpcode(cell opcode);

#endif // !AMXOPCODE_H
