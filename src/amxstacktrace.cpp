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

#include <algorithm>
#include <cassert>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "amxdebuginfo.h"
#include "amxopcode.h"
#include "amxscript.h"
#include "amxstacktrace.h"

namespace {

bool IsStackAddress(AMXScript amx, cell address) {
  return address >= amx.GetHlw() && address < amx.GetStp();
}

bool IsDataAddress(AMXScript amx, cell address) {
  return address >= 0 && address < amx.GetStp();
}

bool IsCodeAddress(AMXScript amx, cell address) {
  const AMX_HEADER *hdr = amx.GetHeader();
  return address >= 0 && address < hdr->dat - hdr->cod;
}

bool IsPublicFunction(AMXScript amx, cell address) {
  return amx.FindPublic(address) != 0;
}

bool IsMain(AMXScript amx, cell address) {
  const AMX_HEADER *hdr = amx.GetHeader();
  return address == hdr->cip;
}

cell GetReturnAddress(AMXScript amx, cell frame_address) {
  return *reinterpret_cast<cell*>(amx.GetData() + frame_address
                                  + sizeof(cell));
}

cell GetReturnAddressSafe(AMXScript amx, cell frame_address) {
  if (IsStackAddress(amx, frame_address)) {
    return GetReturnAddress(amx, frame_address);
  }
  return 0;
}

cell GetPreviousFrame(AMXScript amx, cell frame_address) {
  return *reinterpret_cast<cell*>(amx.GetData() + frame_address);
}

cell GetPreviousFrameSafe(AMXScript amx, cell frame_address) {
  if (IsStackAddress(amx, frame_address)) {
    return GetPreviousFrame(amx, frame_address);
  }
  return 0;
}

cell GetCalleeAddress(AMXScript amx, cell return_address) {
  cell code_start = reinterpret_cast<cell>(amx.GetCode());
  cell target_address_offset = code_start + return_address - sizeof(cell);
  return *reinterpret_cast<cell*>(target_address_offset) - code_start;
}

cell GetCalleeAddressSafe(AMXScript amx, cell return_address) {
  if (IsCodeAddress(amx, return_address)) {
    return GetCalleeAddress(amx, return_address);
  }
  return 0;
}

cell GetCallerAddress(AMXScript amx, cell frame_address) {
  cell prev_frame = GetPreviousFrameSafe(amx, frame_address);
  if (prev_frame != 0) {
    cell return_address = GetReturnAddressSafe(amx, prev_frame);
    if (return_address != 0) {
      return GetCalleeAddressSafe(amx, return_address);
    }
  }
  return 0;
}

} // anonymous namespace

AMXStackFrame::AMXStackFrame(AMXScript amx, cell address)
 : amx_(amx),
   address_(0),
   return_address_(0),
   callee_address_(0),
   caller_address_(0)
{
  if (IsStackAddress(amx_, address)) {
    address_ = address;
  }
  if (address_ != 0) {
    return_address_ = GetReturnAddressSafe(amx_, address_);
    if (return_address_ != 0) {
      callee_address_ = GetCalleeAddressSafe(amx_, return_address_);
      caller_address_ = GetCallerAddress(amx_, address_);
    }
  }
}

AMXStackFrame::AMXStackFrame(AMXScript amx,
                             cell address,
                             cell return_address,
                             cell callee_address,
                             cell caller_address)
 : amx_(amx),
   address_(0),
   return_address_(0),
   callee_address_(0),
   caller_address_(0)
{
  if (IsStackAddress(amx_, address)) {
    address_ = address;
  }
  if (IsCodeAddress(amx_, return_address)) {
    return_address_ = return_address;
  }
  if (IsCodeAddress(amx_, callee_address)) {
    callee_address_ = callee_address;
  }
  if (IsCodeAddress(amx_, caller_address)) {
    caller_address_ = caller_address;
  }
}

AMXStackFrame AMXStackFrame::GetPrevious() const {
  return AMXStackFrame(amx_, GetPreviousFrameSafe(amx_, address_));
}

void AMXStackFrame::Print(std::ostream &stream,
                          const AMXDebugInfo &debug_info) const {
  AMXStackFramePrinter printer(stream, debug_info);
  printer.Print(*this);
}

AMXStackTrace::AMXStackTrace(AMXScript amx, cell frame, int max_depth)
 : current_frame_(amx, frame),
   max_depth_(max_depth),
   frame_index_(0)
{
}

bool AMXStackTrace::MoveNext() {
  if (frame_index_ < max_depth_) {
    current_frame_ = current_frame_.GetPrevious();
    frame_index_++;
    return true;
  }
  return false;
}

AMXStackTrace GetAMXStackTrace(AMXScript amx,
                               cell frm,
                               cell cip,
                               int max_depth) {
  if (amx.IsStackOK() && amx.GetStackSpaceLeft() >= 8) {
    amx.PushStack(cip);
    amx.PushStack(frm);
    amx.SetFrm(amx.GetStk());
  }

  AMXStackTrace trace(amx, amx.GetFrm(), max_depth);

  if (amx.IsStackOK()) {
    amx.PopStack();
    amx.PopStack();
    amx.SetFrm(frm);
  }

  return trace;
}

namespace {

class IsArgumentOf : public std::unary_function<AMXDebugSymbol, bool> {
 public:
  IsArgumentOf(cell function_address) : function_address_(function_address) {}
  bool operator()(AMXDebugSymbol symbol) const {
    return symbol.IsLocal() && symbol.GetCodeStart() == function_address_;
  }
 private:
  cell function_address_;
};

cell GetArgumentValue(AMXScript amx, cell frame_address, int index) {
  cell data = reinterpret_cast<cell>(amx.GetData());
  cell arg_offset = data + frame_address + (3 + index) * sizeof(cell);
  return *reinterpret_cast<cell*>(arg_offset);
}

cell GetArgumentValue(const AMXStackFrame &frame, int index) {
  return GetArgumentValue(frame.amx(), frame.address(), index);
}

cell GetNumArgs(AMXScript amx, cell frame_address) {
  cell data = reinterpret_cast<cell>(amx.GetData());
  cell num_args_offset = data + frame_address + 2 * sizeof(cell);
  return *reinterpret_cast<cell*>(num_args_offset) / sizeof(cell);
}

bool IsPrintableChar(char c) {
  return (c >= 32 && c <= 126);
}

char IsPrintableChar(cell c) {
  return IsPrintableChar(static_cast<char>(c & 0xFF));
}

cell *GetDataPtr(AMXScript amx, cell address) {
  if (IsDataAddress(amx, address)) {
    return reinterpret_cast<cell*>(amx.GetData() + address);
  }
  return 0;
}

bool IsPackedString(const cell *string) {
  return *reinterpret_cast<const ucell*>(string) > UNPACKEDMAX;
}

cell GetMaxStringSize(AMXScript amx, cell address) {
  return amx.GetHeader()->stp - address;
}

std::string GetPackedString(const cell *string, std::size_t size) {
  std::string s;
  for (std::size_t i = 0; i < size; i++) {
    cell cp = string[i / sizeof(cell)] >>
              ((sizeof(cell) - i % sizeof(cell) - 1) * 8);
    char cu = IsPrintableChar(cp) ? cp : '\0';
    if (cu == '\0') {
      break;
    }
    s.push_back(cu);
  }
  return s;
}

std::string GetUnpackedString(const cell *string, std::size_t size) {
  std::string s;
  for (std::size_t i = 0; i < size; i++) {
    char c = IsPrintableChar(string[i]) ? string[i] : '\0';
    if (c == '\0') {
      break;
    }
    s.push_back(c);
  }
  return s;
}

void GetStringContents(AMXScript amx, cell address, std::size_t size,
                       std::string &string, bool &packed) {

  cell *ptr = GetDataPtr(amx, address);
  if (ptr != 0) {
    packed = IsPackedString(ptr);
    if (size == 0) {
     size = GetMaxStringSize(amx, address);
    }
    if (packed) {
      string = GetPackedString(ptr, size);
    } else {
      string = GetUnpackedString(ptr, size);
    }
  }
}

cell GetStateVarAddress(AMXScript amx, cell function_address) {
  if (IsCodeAddress(amx, function_address) &&
      IsCodeAddress(amx, function_address + sizeof(cell))) {
    cell opcode = *reinterpret_cast<cell*>(amx.GetCode() + function_address);
    if (opcode == RelocateAMXOpcode(AMX_OP_LOAD_PRI)) {
      return *reinterpret_cast<cell*>(amx.GetCode() + function_address
                                      + sizeof(cell));
    }
  }
  return -1;
}

bool UsesAutomata(const AMXStackFrame &frame) {
  return GetStateVarAddress(frame.amx(), frame.caller_address()) > 0;
}

class CaseTable {
 public:
  struct Record {
    cell value;
    cell address;
  };
  CaseTable(AMXScript amx, cell address) {
    Record *table = reinterpret_cast<Record*>(amx.GetCode()
                                              + address + sizeof(cell));
    int num_records = table[0].value;
    records_.resize(num_records + 1);
    for (int i = 0; i <= num_records; i++) {
      records_[i].value = table[i].value;
      records_[i].address = table[i].address
                            - reinterpret_cast<cell>(amx.GetCode());
    }
  }
  int GetNumRecords() const {
    return static_cast<int>(records_.size());
  }
  cell GetValueAt(cell index) const {
    return records_[index].value;
  }
  cell GetAddressAt(cell index) const {
    return records_[index].address;
  }
 private:
  std::vector<Record> records_;
};

cell GetStateTableAddress(AMXScript amx, cell function_address) {
  return function_address + 4 * sizeof(cell);
}

cell GetStateTableAddressSafe(AMXScript amx, cell function_address) {
  cell state_table_address = GetStateTableAddress(amx, function_address);
  if (IsCodeAddress(amx, state_table_address)) {
    return state_table_address;
  }
  return 0;
}

cell GetRealFunctionAddress(AMXScript amx, cell function_address,
                                           cell return_address) {
  cell state_table_address = GetStateTableAddressSafe(amx, function_address);
  if (state_table_address != 0) {
    CaseTable state_table(amx, state_table_address);
    for (int i = 0; i < state_table.GetNumRecords(); i++) {
      if (state_table.GetAddressAt(i) > return_address) {
        return state_table.GetAddressAt(i - (i > 0));
      }
    }
  }
  return -1;
}

std::vector<cell> GetStateIDs(AMXScript amx, cell function_address,
                                             cell return_address) {
  std::vector<cell> states;

  cell real_address = GetRealFunctionAddress(amx, function_address,
                                                  return_address);
  if (real_address == 0) {
    return states;
  }

  cell state_table_address = GetStateTableAddressSafe(amx, function_address);
  if (state_table_address == 0) {
    return states;
  }

  CaseTable state_table(amx, state_table_address);
  for (int i = 0; i < state_table.GetNumRecords(); i++) {
    if (state_table.GetAddressAt(i) == real_address) {
      if (i > 0) {
        states.push_back(state_table.GetValueAt(i));
      } else {
        states.push_back(0); // fallback
      }
    } else {
      if (states.size() > 0) {
        // State table entries appear to be sorted by address, so if
        // this one doesn't match here's no point in checking the rest.
        break;
      }
    }
  }
  return states;
}

} // anonymous namespace

AMXStackFramePrinter::AMXStackFramePrinter(std::ostream &stream,
                                           const AMXDebugInfo &debug_info)
  : stream_(stream),
    debug_info_(debug_info)
{
}

void AMXStackFramePrinter::Print(const AMXStackFrame &frame) {
  PrintReturnAddress(frame);
  stream_ << " in ";

  PrintCallerNameAndArguments(frame);

  if (debug_info_.IsLoaded() && UsesAutomata(frame)) {
    stream_ << " ";
    PrintState(frame);
  }

  if (debug_info_.IsLoaded() && frame.return_address() != 0) {
    stream_ << " at ";
    PrintSourceLocation(frame.return_address());
  }
}

void AMXStackFramePrinter::PrintTag(const AMXDebugSymbol &symbol) {
  std::string tag_name = debug_info_.GetTagName(symbol.GetTag());
  if (!tag_name.empty() && tag_name != "_") {
    stream_ << tag_name << ":";
  }
}

void AMXStackFramePrinter::PrintAddress(cell address) {
  char old_fill = stream_.fill('0');
  stream_ << std::hex << std::setw(sizeof(cell) * 2) << address << std::dec;
  stream_.fill(old_fill);
}

void AMXStackFramePrinter::PrintReturnAddress(const AMXStackFrame &frame) {
  PrintAddress(frame.return_address());
}

void AMXStackFramePrinter::PrintCallerName(const AMXStackFrame &frame) {
  if (IsMain(frame.amx(), frame.caller_address())) {
    stream_ << "main";
    return;
  }

  if (debug_info_.IsLoaded()) {
    AMXDebugSymbol caller =
      debug_info_.GetExactFunction(frame.caller_address());
    if (caller) {
      if (IsPublicFunction(frame.amx(), caller.GetCodeStart())
          && !IsMain(frame.amx(), caller.GetCodeStart())) {
        stream_ << "public ";
      }
      PrintTag(caller);
      stream_ << caller.GetName();
      return;
    }
  }

  const char *name = 0;
  if (frame.caller_address() != 0) {
    name = frame.amx().FindPublic(frame.caller_address());
  }
  if (name != 0) {
    stream_ << "public " << name;
  } else {
    stream_ << "??";
  }
}

void AMXStackFramePrinter::PrintCallerNameAndArguments(const AMXStackFrame &frame) {
  PrintCallerName(frame);
  stream_ << " (";
  PrintArgumentList(frame);
  stream_ << ")";
}

void AMXStackFramePrinter::PrintArgument(const AMXStackFrame &frame, int index) {
  PrintArgumentValue(frame, index);
}

void AMXStackFramePrinter::PrintArgument(const AMXStackFrame &frame,
                                         const AMXDebugSymbol &arg,
                                         int index) {
  if (arg.IsReference()) {
    stream_ << "&";
  }

  PrintTag(arg);
  stream_ << arg.GetName();

  if (!arg.IsVariable()) {
    std::vector<AMXDebugSymbolDim> dims = arg.GetDims();

    if (arg.IsArray() || arg.IsArrayRef()) {
      for (std::size_t i = 0; i < dims.size(); ++i) {
        if (dims[i].GetSize() == 0) {
          stream_ << "[]";
        } else {
          std::string tag = debug_info_.GetTagName(dims[i].GetTag()) + ":";
          if (tag == "_:") tag.clear();
          stream_ << "[" << tag << dims[i].GetSize() << "]";
        }
      }
    }
  }

  stream_ << "=";
  PrintArgumentValue(frame, arg, index);
}

void AMXStackFramePrinter::PrintValue(const std::string &tag_name, cell value) {
  if (tag_name == "bool") {
    stream_ << (value ? "true" : "false");
  } else if (tag_name == "Float") {
    stream_ << std::fixed << std::setprecision(5) << amx_ctof(value);
  } else {
    stream_ << value;
  }
}

void AMXStackFramePrinter::PrintArgumentValue(const AMXStackFrame &frame,
                                              int index) {
  stream_ << GetArgumentValue(frame, index);
}

void AMXStackFramePrinter::PrintArgumentValue(const AMXStackFrame &frame,
                                              const AMXDebugSymbol &arg,
                                              int index) {
  std::string tag_name = debug_info_.GetTagName(arg.GetTag());
  cell value = GetArgumentValue(frame, index);

  if (arg.IsVariable()) {
    PrintValue(tag_name, value);
    return;
  }

  stream_ << "@";
  PrintAddress(value);

  if (arg.IsReference()) {
    if (cell *ptr = GetDataPtr(frame.amx(), value)) {
      stream_ << " ";
      PrintValue(tag_name, *ptr);
    }
    return;
  }

  if (arg.IsArray() || arg.IsArrayRef()) {
    std::vector<AMXDebugSymbolDim> dims = arg.GetDims();

    // Try to filter out non-printable arrays (e.g. non-strings).
    // This doesn't work 100% of the time, but it's better than nothing.
    if (dims.size() == 1
        && tag_name == "_"
        && debug_info_.GetTagName(dims[0].GetTag()) == "_")
    {
      std::string string;
      bool packed;

      GetStringContents(frame.amx(), value, dims[0].GetSize(), string, packed);
      stream_ << (packed ? " !" : " ");

      static const std::size_t kMaxString = 80;
      if (string.length() > kMaxString) {
        string.replace(kMaxString, string.length() - kMaxString, "...");
      }

      stream_ << "\"" << string << "\"";
    }
  }
}

void AMXStackFramePrinter::PrintVariableArguments(int number) {
  assert(number > 0);
  stream_ << "... <" << number << " ";
  if (number <= 1) {
    stream_ << "argument";
  } else {
    stream_ << "arguments";
  }
  stream_ << ">";
}

void AMXStackFramePrinter::PrintArgumentList(const AMXStackFrame &frame) {
  AMXStackFrame prev_frame = frame.GetPrevious();

  if (prev_frame.address() != 0) {
    // Although the symbol's code start address points at the state
    // switch code block, function arguments actually use the real
    // function address for the code start because in different states
    // they may be not the same.
    cell arg_address = frame.caller_address();
    if (UsesAutomata(frame)) {
      arg_address = GetRealFunctionAddress(frame.amx(), frame.caller_address(),
                                                        frame.return_address());
    }

    std::vector<AMXDebugSymbol> args;
    int num_actual_args = 0;

    if (debug_info_.IsLoaded()) {
      std::remove_copy_if(debug_info_.GetSymbols().begin(),
                          debug_info_.GetSymbols().end(),
                          std::back_inserter(args),
                          std::not1(IsArgumentOf(arg_address)));
      std::sort(args.begin(), args.end());
      num_actual_args = static_cast<int>(args.size());
    } else {
      static const int kMaxRawArgs = 10;
      num_actual_args = std::min(kMaxRawArgs,
                                 GetNumArgs(frame.amx(), prev_frame.address()));
    }

    // Print a comma-separated list of arguments and their values.
    // If debug info is not available argument names are omitted,
    // so only the values are printed.
    for (int i = 0; i < num_actual_args; i++) {
      if (i > 0) {
        stream_ << ", ";
      }
      if (debug_info_.IsLoaded()) {
        PrintArgument(prev_frame, args[i], i);
      } else {
        PrintArgument(prev_frame, i);
      }
    }

    // If the number of actual arguments passed to the function exceeds
    // that obtained via debug info the function may take a variable
    // number of arguments. In this case we don't evaluate them but just
    // just say that they are present as we can't say anything about
    // their names and types.
    int num_var_args = GetNumArgs(frame.amx(), prev_frame.address())
                     - num_actual_args;
    if (num_var_args > 0) {
      if (num_actual_args != 0) {
        stream_ << ", ";
      }
      PrintVariableArguments(num_var_args);
    }
  }
}

void AMXStackFramePrinter::PrintState(const AMXStackFrame &frame) {
  AMXDebugAutomaton automaton = debug_info_.GetAutomaton(
    GetStateVarAddress(frame.amx(), frame.caller_address()));
  if (automaton) {
    std::vector<cell> states = GetStateIDs(frame.amx(), frame.caller_address(),
                                                        frame.return_address());
    if (!states.empty()) {
      stream_ << "<" << automaton.GetName() << ":";
      for (std::size_t i = 0; i < states.size(); i++ ) {
        if (i > 0) {
          stream_ << ", ";
        }
        AMXDebugState state = debug_info_.GetState(automaton.GetID(), states[i]);
        if (state) {
          stream_ << state.GetName();
        }
      }
      stream_ << ">";
    }
  }
}

void AMXStackFramePrinter::PrintSourceLocation(cell address) {
  std::string filename = debug_info_.GetFileName(address);
  if (filename.empty()) {
    filename.assign("<unknown file>");
  }
  stream_ << filename << ":" << debug_info_.GetLineNumber(address) + 1;
}
