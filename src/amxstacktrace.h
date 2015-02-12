// Copyright (c) 2011-2015 Zeex
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

#ifndef AMXSTACKTRACE_H
#define AMXSTACKTRACE_H

#include <iosfwd>

#include "amxscript.h"

class AMXDebugInfo;

class AMXStackFrame {
 public:
  AMXStackFrame(AMXScript amx, cell address);

  AMXStackFrame(AMXScript amx, cell address,
                               cell return_address,
                               cell callee_address,
                               cell caller_address);

  AMXScript &amx() { return amx_; }
  const AMXScript &amx() const { return amx_; }

  cell address() const { return address_; }

  void set_address(cell address) {
    address_ = address; 
  }

  cell return_address() const {return return_address_; }

  void set_return_address(cell return_address) {
    return_address_ = return_address;
  }

  cell callee_address() const { return callee_address_; }

  void set_callee_address(cell callee_address) {
    callee_address_ = callee_address;
  }

  cell caller_address() const { return caller_address_; }

  void set_caller_address(cell caller_address) {
    caller_address_ = caller_address;
  }

  AMXStackFrame GetPrevious() const;

  void Print(std::ostream &stream, const AMXDebugInfo &debug_info) const;

 private:
  AMXScript amx_;
  cell address_;
  cell return_address_;
  cell callee_address_;
  cell caller_address_;
};

class AMXStackTrace {
 public:
  AMXStackTrace(AMXScript amx, cell frame, int max_depth);

  bool MoveNext();

  const AMXStackFrame &current_frame() const {
    return current_frame_;
  }

 private:
  AMXStackFrame current_frame_;
  int max_depth_;
  int frame_index_;
};

AMXStackTrace GetAMXStackTrace(AMXScript amx,
                               cell frm,
                               cell cip,
                               int max_depth);

class AMXStackFramePrinter {
 public:
  AMXStackFramePrinter(std::ostream &stream,
                       const AMXDebugInfo &debug_info);

  void Print(const AMXStackFrame &frame);

  void PrintTag(const AMXDebugSymbol &symbol);

  void PrintAddress(cell address);
  void PrintReturnAddress(const AMXStackFrame &frame);

  void PrintCallerName(const AMXStackFrame &frame);
  void PrintCallerNameAndArguments(const AMXStackFrame &frame);

  void PrintArgument(const AMXStackFrame &frame, int index);
  void PrintArgument(const AMXStackFrame &frame,
                     const AMXDebugSymbol &arg,
                     int index);

  void PrintValue(const std::string &tag_name, cell value);
  void PrintArgumentValue(const AMXStackFrame &frame, int index);
  void PrintArgumentValue(const AMXStackFrame &frame,
                          const AMXDebugSymbol &arg,
                          int index);

  void PrintVariableArguments(int number);

  void PrintArgumentList(const AMXStackFrame &frame);

  void PrintState(const AMXStackFrame &frame);

  void PrintSourceLocation(cell address);

 private:
  std::ostream &stream_;
  const AMXDebugInfo &debug_info_;
};

#endif // !AMXSTACKTRACE_H
