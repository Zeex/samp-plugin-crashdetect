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
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <amx/amx.h>

#include "amxdebuginfo.h"
#include "amxscript.h"
#include "amxstacktrace.h"

class IsArgumentOf : public std::unary_function<AMXDebugSymbol, bool> {
public:
	IsArgumentOf(cell function) 
		: function_(function) {}
	bool operator()(AMXDebugSymbol symbol) const {
		return symbol.IsLocal() && symbol.GetCodeStartAddr() == function_;
	}
private:
	cell function_;
};

AMXStackFrame::AMXStackFrame(AMXScript amx)
	: amx_(amx), frame_addr_(0), ret_addr_(0), func_addr_(0)
{
}

AMXStackFrame::AMXStackFrame(AMXScript amx, cell frame_addr, const AMXDebugInfo *debug_info)
	: amx_(amx), frame_addr_(0), ret_addr_(0), func_addr_(0), debug_info_(debug_info)
{
	cell data = reinterpret_cast<cell>(amx_.GetData());
	cell code = reinterpret_cast<cell>(amx_.GetCode());

	if (IsStackAddr(frame_addr)) {
		cell ret_addr = *(reinterpret_cast<cell*>(data + frame_addr) + 1);
		Init(frame_addr, ret_addr);
	} else {
		Init(frame_addr);
	}
}

AMXStackFrame::AMXStackFrame(AMXScript amx, cell frame_addr, cell ret_addr, const AMXDebugInfo *debug_info)
	: amx_(amx), frame_addr_(0), ret_addr_(0), func_addr_(0), debug_info_(debug_info)
{
	Init(frame_addr, ret_addr);
}

AMXStackFrame::AMXStackFrame(AMXScript amx, cell frame_addr, cell ret_addr, cell func_addr, const AMXDebugInfo *debug_info)
	: amx_(amx), frame_addr_(0), ret_addr_(0), func_addr_(0), debug_info_(debug_info)
{
	Init(frame_addr, ret_addr, func_addr);
}

void AMXStackFrame::Init(cell frame_addr, cell ret_addr, cell func_addr) {
	if (IsStackAddr(frame_addr)) {
		frame_addr_ = frame_addr;
	}
	if (IsCodeAddr(ret_addr)) {
		ret_addr_ = ret_addr;
	}
	if (IsCodeAddr(func_addr)) {
		func_addr_ = func_addr;
	}

	if (func_addr_ == 0) {
		AMXDebugSymbol func = GetFuncSymbol();
		if (func) {
			func_addr_ = func.GetCodeStartAddr();
		}
	}
}

AMXStackFrame::~AMXStackFrame() {
}

cell AMXStackFrame::GetArgValue(int index) const {
	const unsigned char *data = amx_.GetData();
	return *reinterpret_cast<const cell*>(data + frame_addr_ + (3 + index) * sizeof(cell));
}

int AMXStackFrame::GetNumArgs() const {
	if (frame_addr_ > 0) {
		const unsigned char *data = amx_.GetData();
		return *reinterpret_cast<const cell*>(data + frame_addr_ + 2*sizeof(cell)) / sizeof(cell);
	}
	return -1;
}

AMXStackFrame AMXStackFrame::GetNextFrame() const {
	if (frame_addr_ != 0 && ret_addr_ != 0) {
		const unsigned char *data = amx_.GetData();
		return AMXStackFrame(amx_, *reinterpret_cast<const cell*>(data + frame_addr_), debug_info_);
	} else {
		return AMXStackFrame(amx_);
	}
}

AMXDebugSymbol AMXStackFrame::GetFuncSymbol() const {
	AMXDebugSymbol func;
	if (HasDebugInfo() && ret_addr_ != 0) {
		func = debug_info_->GetFunc(ret_addr_);
	}
	return func;
}

void AMXStackFrame::GetArgSymbols(std::vector<AMXDebugSymbol> &args) const {
	if (AMXDebugSymbol func = GetFuncSymbol()) {
		std::remove_copy_if(
			debug_info_->GetSymbols().begin(),
			debug_info_->GetSymbols().end(),
			std::back_inserter(args),
			std::not1(IsArgumentOf(func.GetCodeStartAddr()))
		);
		std::sort(args.begin(), args.end());
	}
}

std::string AMXStackFrame::AsString() const {
	std::stringstream stream;

	AMXDebugSymbol func = GetFuncSymbol();

	if (ret_addr_ == 0) {
		stream << "???????? in ";
	} else {
		stream << std::hex << std::setw(8) << std::setfill('0') 
			<< ret_addr_ << std::dec << " in ";
	}

	if (func) {
		if (IsPublicFuncAddr(func_addr_) && !IsMainAddr(func_addr_)) {
			stream << "public ";
		}
		std::string func_tag = debug_info_->GetTagName((func).GetTag());
		if (!func_tag.empty() && func_tag != "_") {
			stream << func_tag << ":";
		}		
		stream << debug_info_->GetFuncName(func_addr_);
	} else {		
		const char *name = amx_.FindPublic(func_addr_);
		if (name != 0) {
			if (!IsMainAddr(func_addr_)) {
				stream << "public ";
			}
			stream << name;
		} else {
			if (func_addr_ == amx_.GetHeader()->cip) {
				stream << "main";
			} else {
				stream << "??"; // unknown function
			}
		}
	}

	stream << " (";

	if (func && frame_addr_ != 0) {
		AMXStackFrame next_frame = GetNextFrame();

		std::vector<AMXDebugSymbol> args;
		GetArgSymbols(args);

		// Build a comma-separated list of arguments and their values.
		for (std::size_t i = 0; i < args.size(); i++) {
			AMXDebugSymbol &arg = args[i];

			if (i != 0) {
				stream << ", ";
			}

			if (arg.IsReference()) {
				stream << "&";
			}

			std::string tag = debug_info_->GetTag(arg.GetTag()).GetName() + ":";
			if (tag == "_:") {
				stream << arg.GetName();
			} else {
				stream << tag << arg.GetName();
			}

			cell value = next_frame.GetArgValue(i);
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
							std::string tag = debug_info_->GetTagName(dims[i].GetTag()) + ":";
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
						&& debug_info_->GetTagName(dims[0].GetTag()) == "_")
				{
					std::pair<std::string, bool> s = GetAMXString(value, dims[0].GetSize());
					stream << " ";
					if (s.second) {
						stream << "!"; // packed string
					}
					if (s.first.length() > kMaxString) {
						// The text appears to be overly long for us.
						s.first.replace(kMaxString, s.first.length() - kMaxString, "...");
					}
					stream << "\"" << s.first << "\"";
				}
			}
		}	

		int num_args = static_cast<int>(args.size());
		int num_var_args = next_frame.GetNumArgs() - num_args;

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

	if (HasDebugInfo() && ret_addr_ != 0) {
		std::string fileName = debug_info_->GetFileName(ret_addr_);
		if (!fileName.empty()) {
			stream << " at " << fileName;
		}
		long line = debug_info_->GetLineNo(ret_addr_);
		if (line != 0) {
			stream << ":" << line;
		}
	}

	return stream.str();
}

bool AMXStackFrame::IsPublicFuncAddr(cell address) const {
	return amx_.FindPublic(address) != 0;
}

bool AMXStackFrame::IsMainAddr(cell address) const {
	const AMX_HEADER *hdr = amx_.GetHeader();
	return address == hdr->cip;
}

bool AMXStackFrame::IsStackAddr(cell address) const {
	return (address >= amx_.GetHlw() && address < amx_.GetStp());
}

bool AMXStackFrame::IsDataAddr(cell address) const {
	return address < amx_.GetStp();
}

bool AMXStackFrame::IsCodeAddr(cell address) const {
	const AMX_HEADER *hdr = amx_.GetHeader();
	return address < hdr->dat - hdr->cod;
}

inline bool IsPrintableChar(char c) {
	return (c >= 32 && c <= 126);
}

inline char IsPrintableChar(cell c) {
	return IsPrintableChar(static_cast<char>(c & 0xFF));
}

std::string AMXStackFrame::GetPackedAMXString(const cell *string, std::size_t size) const {
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

std::string AMXStackFrame::GetUnpackedAMXString(const cell *string, std::size_t size) const {
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

std::pair<std::string, bool> AMXStackFrame::GetAMXString(cell address, std::size_t size) const {
	std::pair<std::string, bool> result = std::make_pair("", false);

	const AMX_HEADER *hdr = amx_.GetHeader();

	if (!IsDataAddr(address)) {
		return result;
	}

	const cell *cstr = reinterpret_cast<const cell*>(amx_.GetData() + address);

	if (size == 0) {
		// Size is unknown - copy up to the end of data.
		size = hdr->stp - address;
	}

	if (*reinterpret_cast<const cell*>(cstr) > UNPACKEDMAX) {
		result.first = GetPackedAMXString(cstr, size);
		result.second = true;
	} else {
		result.first = GetUnpackedAMXString(cstr, size);
		result.second = false;
	}

	return result;
}

AMXStackTrace::AMXStackTrace(AMX *amx, const AMXDebugInfo *debug_info)
	: frame_(amx, amx->frm, debug_info)
{
}

AMXStackTrace::AMXStackTrace(AMX *amx, cell frame_addr, const AMXDebugInfo *debug_info)
	: frame_(amx, frame_addr, debug_info)
{
}

bool AMXStackTrace::Next() {
	if (frame_) {
		frame_ = frame_.GetNextFrame();
		if (frame_.GetRetAddr() == 0) {
			return false;
		}
		return true;
	}
	return false;
}
