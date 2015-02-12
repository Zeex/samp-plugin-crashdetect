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

#ifndef AMXSCRIPT_H
#define AMXSCRIPT_H

#include <amx/amx.h>

#include "amxerror.h"

class AMXScript {
 public:
  AMXScript(AMX *amx);

  operator AMX*()       { return amx_; }
  operator AMX*() const { return amx_; }

  bool operator==(const AMX *rhs) const {
    return amx_ == rhs;
  }
  bool operator==(AMXScript rhs) const {
    return amx_ == rhs.amx_;
  }

  bool operator!=(const AMX *rhs) const {
    return amx_ != rhs;
  }
  bool operator!=(AMXScript rhs) const {
    return amx_ != rhs.amx_;
  }

  AMXError GetError() const { return AMXError(amx_->error); }

  uint16_t GetFlags() const { return amx_->flags; }
  void SetFlags(uint16_t flags) { amx_->flags = flags; }

  AMX_DEBUG GetDebugHook() { return amx_->debug; }
  void SetDebugHook(AMX_DEBUG debug) { amx_SetDebugHook(amx_, debug); }

  AMX_CALLBACK GetCallback() { return amx_->callback; }
  void SetCallback(AMX_CALLBACK callback) { amx_SetCallback(amx_, callback); }

  cell GetCip() const { return amx_->cip; }
  cell GetFrm() const { return amx_->frm; }
  cell GetHea() const { return amx_->hea; }
  cell GetHlw() const { return amx_->hlw; }
  cell GetStk() const { return amx_->stk; }
  cell GetStp() const { return amx_->stp; }
  cell GetPri() const { return amx_->pri; }
  cell GetAlt() const { return amx_->alt; }

  void SetFrm(cell frm) { amx_->frm = frm; }
  void SetHea(cell hea) { amx_->hea = hea; }
  void SetStk(cell stk) { amx_->stk = stk; }
  void SetPri(cell pri) { amx_->pri = pri; }
  void SetAlt(cell alt) { amx_->alt = alt; }

  AMX *amx()       { return amx_; }
  AMX *amx() const { return amx_; }

  AMX_HEADER *GetHeader() {
    return const_cast<AMX_HEADER*>(const_this()->GetHeader());
  }
  const AMX_HEADER *GetHeader() const;

  unsigned char *GetData() {
    return const_cast<unsigned char*>(const_this()->GetData());
  }
  const unsigned char *GetData() const;

  unsigned char *GetCode() {
    return const_cast<unsigned char*>(const_this()->GetCode());
  }
  const unsigned char *GetCode() const;

  AMX_FUNCSTUBNT *GetNatives() {
    return const_cast<AMX_FUNCSTUBNT*>(const_this()->GetNatives());
  }
  const AMX_FUNCSTUBNT *GetNatives() const;

  AMX_FUNCSTUBNT *GetPublics() {
    return const_cast<AMX_FUNCSTUBNT*>(const_this()->GetPublics());
  }
  const AMX_FUNCSTUBNT *GetPublics() const;

  const char *FindPublic(cell address) const;
  const char *FindNative(cell address) const;

  int GetNumNatives() const;
  int GetNumPublics() const;

  cell GetNativeIndex(const char *name) const;
  cell GetPublicIndex(const char *name) const;

  cell GetNativeAddress(int index) const;
  cell GetPublicAddress(int index) const;

  const char *GetName(uint32_t offset) const;
  const char *GetNativeName(int index) const;
  const char *GetPublicName(int index) const;

  cell GetStackSpaceLeft() const;
  bool IsStackOK() const;

  void PushStack(cell value);
  cell PopStack();
  void PopStack(int ncells);

  void DisableSysreqD();

 private:
  const AMXScript *const_this() {
    return static_cast<const AMXScript*>(this);
  }

 private:
  AMX *amx_;
};

#endif // !AMXSCRIPT_H
