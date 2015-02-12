// Copyright (c) 2014-2015 Zeex
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

#ifndef AMXCALLSTACK_H
#define AMXCALLSTACK_H

#include <stack>

#include "amxscript.h"

class AMXCall {
 public:
  enum Type {
    NATIVE,
    PUBLIC
  };

  AMXCall(Type type, AMXScript amx, cell index);
  AMXCall(Type type, AMXScript amx, cell index, cell frm, cell cip);

  static AMXCall Public(AMXScript amx, cell index);
  static AMXCall Native(AMXScript amx, cell index);

  AMXScript amx() const { return amx_; }

  Type type()  const { return type_;  }
  cell index() const { return index_; }
  cell frm()   const { return frm_;   }
  cell cip()   const { return cip_;   }

  bool IsPublic() const { return type_ == PUBLIC; }
  bool IsNative() const { return type_ == NATIVE; }

 private:
  AMXScript amx_;
  Type type_;
  cell frm_;
  cell cip_;
  cell index_;
};

class AMXCallStack {
 public:
  bool IsEmpty() const;

  AMXCall &Top();
  const AMXCall &Top() const;

  void Push(AMXCall call);
  AMXCall Pop();

 private:
  std::stack<AMXCall> call_stack_;
};

#endif // !AMXCALLSTACK_H
