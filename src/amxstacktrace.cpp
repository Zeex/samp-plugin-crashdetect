// Copyright (c) 2011-2013 Zeex
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
#include <utility>
#include <vector>

#include "amxdebuginfo.h"
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

class IsArgumentOf : public std::unary_function<AMXDebugSymbol, bool> {
 public:
  IsArgumentOf(cell function_address) : function_address_(function_address) {}
  bool operator()(AMXDebugSymbol symbol) const {
    return symbol.IsLocal() && symbol.GetCodeStart() == function_address_;
  }
 private:
  cell function_address_;
};

cell GetArgValue(AMXScript amx, cell frame, int index) {
  cell data = reinterpret_cast<cell>(amx.GetData());
  cell arg_offset = data + frame + (3 + index) * sizeof(cell);
  return *reinterpret_cast<cell*>(arg_offset);
}

bool IsPrintableChar(char c) {
  return (c >= 32 && c <= 126);
}

char IsPrintableChar(cell c) {
  return IsPrintableChar(static_cast<char>(c & 0xFF));
}

std::string GetPackedString(const cell *string, std::size_t size) {
  std::string s;
  for (std::size_t i = 0; i < size; i++) {
    cell cp = string[i / sizeof(cell)]
              >> ((sizeof(cell) - i % sizeof(cell) - 1) * 8);
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

cell GetNumArgs(AMXScript amx, cell frame_address) {
  cell data = reinterpret_cast<cell>(amx.GetData());
  cell num_args_offset = data + frame_address + 2 * sizeof(cell);
  return *reinterpret_cast<cell*>(num_args_offset) / sizeof(cell);
}

cell GetNumArgsSafe(AMXScript amx, cell frame_address) {
  if (IsStackAddress(amx, frame_address)) {
    return GetNumArgs(amx, frame_address);
  }
  return 0;
}

std::pair<std::string, bool> GetStringContents(AMXScript amx, cell address,
                                               std::size_t size) {
  std::pair<std::string, bool> result = std::make_pair("", false);

  const AMX_HEADER *hdr = amx.GetHeader();

  if (!IsDataAddress(amx, address)) {
    return result;
  }

  const cell *cstr = reinterpret_cast<const cell*>(amx.GetData() + address);

  if (size == 0) {
    size = hdr->stp - address;
  }

  if (*reinterpret_cast<const cell*>(cstr) > UNPACKEDMAX) {
    result.first = GetPackedString(cstr, size);
    result.second = true;
  } else {
    result.first = GetUnpackedString(cstr, size);
    result.second = false;
  }

  return result;
}

cell GetStateVarAddress(AMXScript amx, cell function_address) {
  static const int load_pri = 1;
  if (IsCodeAddress(amx, function_address) &&
      IsCodeAddress(amx, function_address + sizeof(cell))) {
    cell opcode = *reinterpret_cast<cell*>(amx.GetCode() + function_address);
    if (opcode == load_pri) {
      return *reinterpret_cast<cell*>(amx.GetCode() + function_address
                                      + sizeof(cell));
    }
  }
  return -1;
}

class CaseTable {
 public:
  struct Record {
    cell value;
    cell address;
  };

  CaseTable(AMXScript amx, cell address) {
    Record *case_table = reinterpret_cast<Record*>(amx.GetCode()
                                                   + address + sizeof(cell));
    int num_records = case_table[0].value;

    for (int i = 0; i <= num_records; i++) {
      cell dest = case_table[i].address
                  - reinterpret_cast<cell>(amx.GetCode());
      records_.push_back(std::make_pair(case_table[i].value, dest));
    }
  }

  int GetNumRecords() const {
    return static_cast<int>(records_.size());
  }
  cell GetValueAt(cell index) const {
    return records_[index].first;
  }
  cell GetAddressAt(cell index) const {
    return records_[index].second;
  }

 private:
  std::vector<std::pair<cell, cell> > records_;
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
  cell state_table_address = GetStateTableAddress(amx, function_address);
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
  return AMXStackFrame(amx_, GetPreviousFrame(amx_, address_));
}

void AMXStackFrame::Print(std::ostream &stream,
                          const AMXDebugInfo *debug_info) const {
  bool have_debug_info = debug_info != 0 && debug_info->IsLoaded();
  bool have_states = (GetStateVarAddress(amx_, caller_address_) != 0);

  AMXDebugSymbol caller;
  if (have_debug_info) {
    if (have_states) {
      caller = debug_info->GetExactFunction(caller_address_);
    } else {
      caller = debug_info->GetFunction(return_address_);
    }
  }

  if (return_address_ == 0) {
    stream << "???????? in ";
  } else {
    stream << std::hex << std::setw(8) << std::setfill('0') 
      << return_address_ << std::dec << " in ";
  }

  if (caller) {
    if (IsPublicFunction(amx_, caller.GetCodeStart()) &&
        !IsMain(amx_, caller.GetCodeStart())) {
      stream << "public ";
    }
    std::string func_tag = debug_info->GetTagName((caller).GetTag());
    if (!func_tag.empty() && func_tag != "_") {
      stream << func_tag << ":";
    }    
    stream << caller.GetName();
  } else {
    if (IsMain(amx_, caller_address_)) {
      stream << "main";
    } else {
      const char *name = 0;
      if (caller_address_ != 0) {
        name = amx_.FindPublic(caller_address_);
      }
      if (name != 0) {
        stream << "public " << name;
      } else {
        stream << "??";
      }
    }
  }

  stream << " (";

  if (have_debug_info && caller && address_ != 0) {
    AMXStackFrame prev = GetPrevious();

    // Although the symbol's code start address points at the state
    // switch code block, function arguments actually use the real
    // function address for the code start because in different states
    // they may be not the same.
    cell arg_address = caller_address_;
    if (have_states) {
      arg_address = GetRealFunctionAddress(amx_, caller_address_,
                                                 return_address_);
    }

    std::vector<AMXDebugSymbol> args;
    std::remove_copy_if(debug_info->GetSymbols().begin(),
                        debug_info->GetSymbols().end(),
                        std::back_inserter(args),
                        std::not1(IsArgumentOf(arg_address)));
    std::sort(args.begin(), args.end());

    // Build a comma-separated list of arguments and their values.
    for (std::size_t i = 0; i < args.size(); i++) {
      AMXDebugSymbol &arg = args[i];

      if (i != 0) {
        stream << ", ";
      }

      if (arg.IsReference()) {
        stream << "&";
      }

      std::string tag = debug_info->GetTag(arg.GetTag()).GetName() + ":";
      if (tag == "_:") {
        stream << arg.GetName();
      } else {
        stream << tag << arg.GetName();
      }

      cell value = GetArgValue(amx_, prev.address(), i);
      if (arg.IsVariable()) {
        if (tag == "bool:") {
          stream << "=" << (value ? "true" : "false");
        } else if (tag == "Float:") {
          stream << "=" << std::fixed << std::setprecision(5) << amx_ctof(value);
        } else {
          stream << "=" << value;
        }
      } else {
        std::vector<AMXDebugSymbolDim> dims = arg.GetDims();
        if (arg.IsArray() || arg.IsArrayRef()) {
          for (std::size_t i = 0; i < dims.size(); ++i) {
            if (dims[i].GetSize() == 0) {
              stream << "[]";
            } else {
              std::string tag = debug_info->GetTagName(dims[i].GetTag()) + ":";
              if (tag == "_:") tag.clear();
              stream << "[" << tag << dims[i].GetSize() << "]";
            }
          }
        }

        // For arrays/references we just output their amx_ address.
        stream << "=@0x" << std::hex << std::setw(8) << std::setfill('0')
               << value << std::dec;

        if ((arg.IsArray() || arg.IsArrayRef())
            && dims.size() == 1
            && tag == "_:"
            && debug_info->GetTagName(dims[0].GetTag()) == "_")
        {
          std::pair<std::string, bool> s = GetStringContents(amx_, value,
                                                            dims[0].GetSize());
          stream << " ";
          if (s.second) {
            stream << "!"; // packed string
          }
          if (s.first.length() > kMaxPrintString) {
            // The text appears to be overly long for us.
            s.first.replace(kMaxPrintString,
                            s.first.length() - kMaxPrintString, "...");
          }
          stream << "\"" << s.first << "\"";
        }
      }
    }  

    int num_args = static_cast<int>(args.size());
    int num_var_args = GetNumArgsSafe(amx_, prev.address()) - num_args;

    // If number of arguments passed to the function exceeds that obtained
    // through debug info it's likely that the function takes a variable
    // number of arguments. In this case we don't evaluate them but rather
    // just say that they are present (because we can't say anything about
    // their names and types).
    if (num_var_args > 0) {
      if (num_args != 0) {
        stream << ", ";
      }
      stream << "... <" << num_var_args << " variable ";
      if (num_var_args == 1) {
        stream << "argument";
      } else {
        stream << "arguments";
      }
      stream << ">";
    }
  }

  stream << ")";

  if (caller && have_states) {
    cell sv_address = GetStateVarAddress(amx_, caller_address_);
    AMXDebugAutomaton automaton = debug_info->GetAutomaton(sv_address);
    if (automaton) {
      std::vector<cell> states = GetStateIDs(amx_, caller_address_, return_address_);
      if (!states.empty()) {
        stream << " <";
        for (std::size_t i = 0; i < states.size(); i++ ) {
          static const char *separator = "";
          AMXDebugState state = debug_info->GetState(automaton.GetID(), states[i]);
          if (state) {
            stream << separator << automaton.GetName() << ":" << state.GetName();
          }
          separator = ", ";
        }
        stream << ">";
      }
    }
  }

  if (have_debug_info && return_address_ != 0) {
    std::string filename = debug_info->GetFileName(return_address_);
    if (!filename.empty()) {
      stream << " at " << filename;
    }
    long line = debug_info->GetLineNumber(return_address_);
    if (line >= 0) {
      stream << ":" << line + 1;
    }
  }
}

AMXStackTrace::AMXStackTrace(AMXScript amx)
 : current_frame_(amx, amx.GetFrm())
{
}

AMXStackTrace::AMXStackTrace(AMXScript amx, cell frame)
 : current_frame_(amx, frame)
{
}

bool AMXStackTrace::Next() {
  if (current_frame_) {
    current_frame_ = current_frame_.GetPrevious();
    if (current_frame_.return_address() == 0) {
      return false;
    }
    return true;
  }
  return false;
}
