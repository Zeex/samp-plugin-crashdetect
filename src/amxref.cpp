// Copyright (c) 2012-2020 Zeex
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
#include "amxref.h"

AMXRef::AMXRef(AMX *amx)
 : amx_(amx)
{
}

AMX_HEADER *AMXRef::GetHeader() const {
  return reinterpret_cast<AMX_HEADER*>(amx_->base);
}

unsigned char *AMXRef::GetData() const {
  unsigned char *data = amx_->data;
  if (data == nullptr) {
    AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx_->base);
    data = amx_->base + hdr->dat;
  }
  return data;
}

unsigned char *AMXRef::GetCode() const {
  AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx_->base);
  unsigned char *code = amx_->base + hdr->cod;
  return code;
}


AMX_FUNCSTUBNT *AMXRef::GetNatives() const {
  const AMX_HEADER *hdr = GetHeader();
  return reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->natives + amx_->base);
}

AMX_FUNCSTUBNT *AMXRef::GetPublics() const {
  const AMX_HEADER *hdr = GetHeader();
  return reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->publics + amx_->base);
}

const char *AMXRef::FindPublic(cell address) const {
  const AMX_FUNCSTUBNT *publics = GetPublics();
  int n = GetNumPublics();
  for (int i = 0; i < n; i++) {
    if (publics[i].address == static_cast<ucell>(address)) {
      return GetString(publics[i].nameofs);
    }
  }
  return nullptr;
}

const char *AMXRef::FindNative(cell address) const {
  const AMX_FUNCSTUBNT *natives = GetNatives();
  int n = GetNumNatives();
  for (int i = 0; i < n; i++) {
    if (natives[i].address == static_cast<ucell>(address)) {
      return GetString(natives[i].nameofs);
    }
  }
  return nullptr;
}

int AMXRef::GetNumNatives() const {
  const AMX_HEADER *hdr = GetHeader();
  return (hdr->libraries - hdr->natives) / hdr->defsize;
}

int AMXRef::GetNumPublics() const {
  const AMX_HEADER *hdr = GetHeader();
  return (hdr->natives - hdr->publics) / hdr->defsize;
}

cell AMXRef::GetNativeIndex(const char *name) const {
  int n = GetNumNatives();
  const AMX_FUNCSTUBNT *natives = GetNatives();
  for (int i = 0; i < n; i++) {
    if (std::strcmp(GetString(natives[i].nameofs), name) == 0) {
      return i;
    }
  }
  return -1;
}

cell AMXRef::GetPublicIndex(const char *name) const {
  int n = GetNumPublics();
  const AMX_FUNCSTUBNT *publics = GetPublics();
  for (int i = 0; i < n; i++) {
    if (std::strcmp(GetString(publics[i].nameofs), name) == 0) {
      return i;
    }
  }
  return -1;
}

cell AMXRef::GetNativeAddress(int index) const {
  int n = GetNumNatives();
  const AMX_FUNCSTUBNT *natives = GetNatives();
  if (index >= 0 && index < n) {
    return natives[index].address;
  }
  return 0;
}

cell AMXRef::GetPublicAddress(int index) const {
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

const char *AMXRef::GetNativeName(int index) const {
  int n = GetNumNatives();
  const AMX_FUNCSTUBNT *natives = GetNatives();
  if (index >= 0 && index < n) {
    return GetString(natives[index].nameofs);
  }
  return nullptr;
}

const char *AMXRef::GetPublicName(int index) const {
  int n = GetNumPublics();
  const AMX_FUNCSTUBNT *publics = GetPublics();
  if (index >=0 && index < n) {
    return GetString(publics[index].nameofs);
  } else if (index == AMX_EXEC_MAIN) {
    return "main";
  }
  return nullptr;
}

const char *AMXRef::GetString(uint32_t offset) const {
  return reinterpret_cast<char*>(amx_->base + offset);
}

cell AMXRef::GetStackSpaceLeft() const {
  return GetStk() - GetHlw();
}

bool AMXRef::CheckStack() const {
  return GetStk() >= GetHlw() && GetStk() <= GetStp();
}

void AMXRef::PushStack(cell value) {
  amx_->stk -= sizeof(cell);
  *reinterpret_cast<cell*>(GetData() + amx_->stk) = value;
}

cell AMXRef::PopStack() {
  cell value = *reinterpret_cast<cell*>(GetData() + amx_->stk);
  amx_->stk += sizeof(cell);
  return value;
}

void AMXRef::PopStack(int ncells) {
  amx_->stk += sizeof(cell) * ncells;
}
