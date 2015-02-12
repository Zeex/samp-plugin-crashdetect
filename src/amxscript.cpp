// Copyright (c) 2012-2015 Zeex
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

#include <cstring>

#include "amxscript.h"

AMXScript::AMXScript(AMX *amx)
 : amx_(amx)
{
}

const AMX_HEADER *AMXScript::GetHeader() const {
  return reinterpret_cast<AMX_HEADER*>(amx_->base);
}

const unsigned char *AMXScript::GetData() const {
  unsigned char *data = amx_->data;
  if (data == 0) {
    AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx_->base);
    data = amx_->base + hdr->dat;
  }
  return data;
}

const unsigned char *AMXScript::GetCode() const {
  AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx_->base);
  unsigned char *code = amx_->base + hdr->cod;
  return code;
}


const AMX_FUNCSTUBNT *AMXScript::GetNatives() const {
  const AMX_HEADER *hdr = GetHeader();
  return reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->natives + amx_->base);
}

const AMX_FUNCSTUBNT *AMXScript::GetPublics() const {
  const AMX_HEADER *hdr = GetHeader();
  return reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->publics + amx_->base);
}

const char *AMXScript::FindPublic(cell address) const {
  const AMX_FUNCSTUBNT *publics = GetPublics();
  int n = GetNumPublics();
  for (int i = 0; i < n; i++) {
    if (publics[i].address == static_cast<ucell>(address)) {
      return GetName(publics[i].nameofs);
    }
  }
  return 0;
}

const char *AMXScript::FindNative(cell address) const {
  const AMX_FUNCSTUBNT *natives = GetNatives();
  int n = GetNumNatives();
  for (int i = 0; i < n; i++) {
    if (natives[i].address == static_cast<ucell>(address)) {
      return GetName(natives[i].nameofs);
    }
  }
  return 0;
}

int AMXScript::GetNumNatives() const {
  const AMX_HEADER *hdr = GetHeader();
  return (hdr->libraries - hdr->natives) / hdr->defsize;
}

int AMXScript::GetNumPublics() const {
  const AMX_HEADER *hdr = GetHeader();
  return (hdr->natives - hdr->publics) / hdr->defsize;
}

cell AMXScript::GetNativeIndex(const char *name) const {
  int n = GetNumNatives();
  const AMX_FUNCSTUBNT *natives = GetNatives();
  for (int i = 0; i < n; i++) {
    if (std::strcmp(GetName(natives[i].nameofs), name) == 0) {
      return i;
    }
  }
  return -1;
}

cell AMXScript::GetPublicIndex(const char *name) const {
  int n = GetNumPublics();
  const AMX_FUNCSTUBNT *publics = GetPublics();
  for (int i = 0; i < n; i++) {
    if (std::strcmp(GetName(publics[i].nameofs), name) == 0) {
      return i;
    }
  }
  return -1;
}

cell AMXScript::GetNativeAddress(int index) const {
  int n = GetNumNatives();
  const AMX_FUNCSTUBNT *natives = GetNatives();
  if (index >= 0 && index < n) {
    return natives[index].address;
  }
  return 0;
}

cell AMXScript::GetPublicAddress(int index) const {
  int n = GetNumPublics();
  const AMX_FUNCSTUBNT *publics = GetPublics();
  if (index >=0 && index < n) {
    return publics[index].address;
  } else if (index == AMX_EXEC_MAIN) {
    const AMX_HEADER *hdr = GetHeader();
    return hdr->cip;
  }
  return 0;
}

const char *AMXScript::GetNativeName(int index) const {
  int n = GetNumNatives();
  const AMX_FUNCSTUBNT *natives = GetNatives();
  if (index >= 0 && index < n) {
    return GetName(natives[index].nameofs);
  }
  return 0;
}

const char *AMXScript::GetPublicName(int index) const {
  int n = GetNumPublics();
  const AMX_FUNCSTUBNT *publics = GetPublics();
  if (index >=0 && index < n) {
    return GetName(publics[index].nameofs);
  } else if (index == AMX_EXEC_MAIN) {
    return "main";
  }
  return 0;
}

const char *AMXScript::GetName(uint32_t offset) const {
  return reinterpret_cast<char*>(amx_->base + offset);
}

cell AMXScript::GetStackSpaceLeft() const {
  return GetStk() - GetHlw();
}

bool AMXScript::IsStackOK() const {
  return GetStk() >= GetHlw() && GetStk() <= GetStp();
}

void AMXScript::PushStack(cell value) {
  amx_->stk -= sizeof(cell);
  *reinterpret_cast<cell*>(GetData() + amx_->stk) = value;
}

cell AMXScript::PopStack() {
  cell value = *reinterpret_cast<cell*>(GetData() + amx_->stk);
  amx_->stk += sizeof(cell);
  return value;
}

void AMXScript::PopStack(int ncells) {
  amx_->stk += sizeof(cell) * ncells;
}

void AMXScript::DisableSysreqD() {
  amx_->sysreq_d = 0;
}
