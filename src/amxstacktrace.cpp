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

class IsArgumentOf : public std::unary_function<AMXDebugSymbol, bool> {
 public:
  IsArgumentOf(cell function) : function_(function) {}
  bool operator()(AMXDebugSymbol symbol) const {
    return symbol.IsLocal() && symbol.GetCodeStart() == function_;
  }
 private:
  cell function_;
};

bool IsStackAddress(AMXScript amx, cell address) {
  return (address >= amx.GetHlw() && address < amx.GetStp());
}

bool IsDataAddress(AMXScript amx, cell address) {
  return address < amx.GetStp();
}

bool IsCodeAddress(AMXScript amx, cell address) {
  const AMX_HEADER *hdr = amx.GetHeader();
  return address < hdr->dat - hdr->cod;
}

bool IsPublicFunction(AMXScript amx, cell address) {
  return amx.FindPublic(address) != 0;
}

bool IsMain(AMXScript amx, cell address) {
  const AMX_HEADER *hdr = amx.GetHeader();
  return address == hdr->cip;
}

cell GetReturnAddress(AMXScript amx, cell frame) {
  return *reinterpret_cast<cell*>(amx.GetData() + frame + sizeof(cell));
}

cell GetReturnAddressSafe(AMXScript amx, cell frame) {
  if (IsStackAddress(amx, frame)) {
    return GetReturnAddress(amx, frame);
  }
  return 0;
}

cell GetPreviousFrame(AMXScript amx, cell frame) {
  return *reinterpret_cast<cell*>(amx.GetData() + frame);
}

cell GetPreviousFrameSafe(AMXScript amx, cell frame) {
  if (IsStackAddress(amx, frame)) {
    return GetPreviousFrame(amx, frame);
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

cell GetCallerAddress(AMXScript amx, cell frame) {
  cell prev_frame = GetPreviousFrameSafe(amx, frame);
  if (prev_frame != 0) {
    cell return_address = GetReturnAddressSafe(amx, prev_frame);
    if (return_address != 0) {
      return GetCalleeAddressSafe(amx, return_address);
    }
  }
  return 0;
}

cell GetArgValue(AMXScript amx, cell frame, int index) {
  cell data = reinterpret_cast<cell>(amx.GetData());
  cell arg_offset = data + frame + (3 + index) * sizeof(cell);
  return *reinterpret_cast<cell*>(arg_offset);
}

cell GetNumArgs(AMXScript amx, cell frame) {
  cell data = reinterpret_cast<cell>(amx.GetData());
  cell num_args_offset = data + frame + 2 * sizeof(cell);
  return *reinterpret_cast<cell*>(num_args_offset) / sizeof(cell);
}

cell GetNumArgsSafe(AMXScript amx, cell frame) {
  if (IsStackAddress(amx, frame)) {
    return GetNumArgs(amx, frame);
  }
  return 0;
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
    cell cp = string[i / sizeof(cell)] >> ((sizeof(cell) - i % sizeof(cell) - 1) * 8);
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

std::pair<std::string, bool> GetStringContents(AMXScript amx, cell address, std::size_t size) {
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

  AMXDebugSymbol caller;
  if (have_debug_info) {
    caller = debug_info->GetFunction(return_address_);
    assert(!caller ||
           caller_address_ == caller.GetCodeStart() ||
           caller_address_ == 0);
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

    std::vector<AMXDebugSymbol> args;
    std::remove_copy_if(debug_info->GetSymbols().begin(),
                        debug_info->GetSymbols().end(),
                        std::back_inserter(args),
                        std::not1(IsArgumentOf(caller.GetCodeStart())));
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
        stream << "=@0x" << std::hex << std::setw(8) << std::setfill('0') << value << std::dec;

        if ((arg.IsArray() || arg.IsArrayRef())
            && dims.size() == 1
            && tag == "_:"
            && debug_info->GetTagName(dims[0].GetTag()) == "_")
        {
          std::pair<std::string, bool> s = GetStringContents(amx_, value, dims[0].GetSize());
          stream << " ";
          if (s.second) {
            stream << "!"; // packed string
          }
          if (s.first.length() > kMaxPrintString) {
            // The text appears to be overly long for us.
            s.first.replace(kMaxPrintString, s.first.length() - kMaxPrintString, "...");
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
