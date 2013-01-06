// Copyright (c) 2011-2012 Zeex
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

#include "amx.h"
#include "amxstacktrace.h"
#include "amxdebuginfo.h"

class IsArgumentOf : public std::unary_function<AMXDebugSymbol, bool> {
public:
	IsArgumentOf(ucell function) 
		: function_(function) {}
	bool operator()(AMXDebugSymbol symbol) const {
		return symbol.IsLocal() && symbol.GetCodeStartAddr() == function_;
	}
private:
	ucell function_;
};

AMXStackFrame::AMXStackFrame(AMX *amx)
	: amx_(amx), frameAddr_(0), retAddr_(0), funcAddr_(0)
{
}

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frameAddr, const AMXDebugInfo *debugInfo)
	: amx_(amx), frameAddr_(0), retAddr_(0), funcAddr_(0), debugInfo_(debugInfo)
{
	ucell data = reinterpret_cast<ucell>(GetDataPtr());
	ucell code = reinterpret_cast<ucell>(GetCodePtr());

	if (IsStackAddr(frameAddr)) {
		ucell retAddr = *(reinterpret_cast<ucell*>(data + frameAddr) + 1);
		Init(frameAddr, retAddr);
	} else {
		Init(frameAddr);
	}
}

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frameAddr, ucell retAddr, const AMXDebugInfo *debugInfo)
	: amx_(amx), frameAddr_(0), retAddr_(0), funcAddr_(0), debugInfo_(debugInfo)
{
	Init(frameAddr, retAddr);
}

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frameAddr, ucell retAddr, ucell funcAddr, const AMXDebugInfo *debugInfo)
	: amx_(amx), frameAddr_(0), retAddr_(0), funcAddr_(0), debugInfo_(debugInfo)
{
	Init(frameAddr, retAddr, funcAddr);
}

void AMXStackFrame::Init(ucell frameAddr, ucell retAddr, ucell funcAddr) {
	if (IsStackAddr(frameAddr)) {
		frameAddr_ = frameAddr;
	}
	if (IsCodeAddr(retAddr)) {
		retAddr_ = retAddr;
	}
	if (IsCodeAddr(funcAddr)) {
		funcAddr_ = funcAddr;
	}

	if (funcAddr_ == 0) {
		AMXDebugSymbol func = GetFuncSymbol();
		if (func) {
			funcAddr_ = func.GetCodeStartAddr();
		}
	}
}

AMXStackFrame::~AMXStackFrame() {
}

cell AMXStackFrame::GetArgValue(int index) const {
	unsigned char *data = GetDataPtr();
	return *reinterpret_cast<cell*>(data + frameAddr_ + (3 + index) * sizeof(cell));
}

int AMXStackFrame::GetNumArgs() const {
	if (frameAddr_ > 0) {
		unsigned char *data = GetDataPtr();
		return *reinterpret_cast<cell*>(data + frameAddr_ + 2*sizeof(cell)) / sizeof(cell);
	}
	return -1;
}

AMXStackFrame AMXStackFrame::GetNextFrame() const {
	if (frameAddr_ != 0 && retAddr_ != 0) {
		unsigned char *data = GetDataPtr();
		return AMXStackFrame(amx_, *reinterpret_cast<cell*>(data + frameAddr_), debugInfo_);
	} else {
		return AMXStackFrame(amx_);
	}
}

AMXDebugSymbol AMXStackFrame::GetFuncSymbol() const {
	AMXDebugSymbol func;
	if (HasDebugInfo() && retAddr_ != 0) {
		func = debugInfo_->GetFunc(retAddr_);
	}
	return func;
}

void AMXStackFrame::GetArgSymbols(std::vector<AMXDebugSymbol> &args) const {
	if (AMXDebugSymbol func = GetFuncSymbol()) {
		std::remove_copy_if(
			debugInfo_->GetSymbols().begin(),
			debugInfo_->GetSymbols().end(),
			std::back_inserter(args),
			std::not1(IsArgumentOf(func.GetCodeStartAddr()))
		);
		std::sort(args.begin(), args.end());
	}
}

std::string AMXStackFrame::AsString() const {
	std::stringstream stream;

	AMXDebugSymbol func = GetFuncSymbol();

	if (retAddr_ == 0) {
		stream << "???????? in ";
	} else {
		stream << std::hex << std::setw(8) << std::setfill('0') 
			<< retAddr_ << std::dec << " in ";
	}

	if (func) {
		if (IsPublicFuncAddr(funcAddr_) && !IsMainAddr(funcAddr_)) {
			stream << "public ";
		}
		std::string funTag = debugInfo_->GetTagName((func).GetTag());
		if (!funTag.empty() && funTag != "_") {
			stream << funTag << ":";
		}		
		stream << debugInfo_->GetFuncName(funcAddr_);
	} else {		
		const char *name = GetPublicFuncName(funcAddr_);
		if (name != 0) {
			if (!IsMainAddr(funcAddr_)) {
				stream << "public ";
			}
			stream << name;
		} else {
			stream << "??"; // unknown function
		}
	}

	stream << " (";

	if (func && frameAddr_ != 0) {
		AMXStackFrame nextFrame = GetNextFrame();

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

			std::string tag = debugInfo_->GetTag(arg.GetTag()).GetName() + ":";
			if (tag == "_:") {
				stream << arg.GetName();
			} else {
				stream << tag << arg.GetName();
			}

			cell value = nextFrame.GetArgValue(i);
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
							std::string tag = debugInfo_->GetTagName(dims[i].GetTag()) + ":";
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
						&& debugInfo_->GetTagName(dims[0].GetTag()) == "_")
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

		int numArgs = static_cast<int>(args.size());
		int numVarArgs = nextFrame.GetNumArgs() - numArgs;

		// If number of arguments passed to the function exceeds that obtained
		// through debug info it's likely that the function takes a variable
		// number of arguments. In this case we don't evaluate them but rather
		// just say that they are present (because we can't say anything about
		// their names and types).
		if (numVarArgs > 0) {
			if (numArgs != 0) {
				stream << ", ";
			}
			stream << "... <" << numVarArgs << " variable ";
			if (numVarArgs == 1) {
				stream << "argument";
			} else {
				stream << "arguments";
			}
			stream << ">";
		}
	}

	stream << ")";

	if (HasDebugInfo() && retAddr_ != 0) {
		std::string fileName = debugInfo_->GetFileName(retAddr_);
		if (!fileName.empty()) {
			stream << " at " << fileName;
		}
		long line = debugInfo_->GetLineNo(retAddr_);
		if (line != 0) {
			stream << ":" << line;
		}
	}

	return stream.str();
}

unsigned char *AMXStackFrame::GetDataPtr() const{
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx_->base);
	unsigned char *data = amx_->data;
	if (data == 0) {
		data = amx_->base + hdr->dat;
	}
	return data;
}

unsigned char *AMXStackFrame::GetCodePtr() const {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx_->base);
	return amx_->base + hdr->cod;
}

const char *AMXStackFrame::GetPublicFuncName(ucell address) const {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx_->base);

	if (address == hdr->cip) {
		return "main";
	}

	AMX_FUNCSTUBNT *publics = reinterpret_cast<AMX_FUNCSTUBNT*>(amx_->base + hdr->publics);
	AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(amx_->base + hdr->natives);

	for (AMX_FUNCSTUBNT *p = publics; p < natives; ++p) {
		if (p->address == address) {
			return reinterpret_cast<const char*>(p->nameofs + amx_->base);
		}
	}

	return 0;
}

bool AMXStackFrame::IsPublicFuncAddr(ucell address) const {
	return GetPublicFuncName(address) != 0;
}

bool AMXStackFrame::IsMainAddr(ucell address) const {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx_->base);
	return static_cast<cell>(address) == hdr->cip;
}

bool AMXStackFrame::IsStackAddr(ucell address) const {
	return (address >= static_cast<ucell>(amx_->hlw)
		&& address < static_cast<ucell>(amx_->stp));
}

bool AMXStackFrame::IsDataAddr(ucell address) const {
	return address < static_cast<ucell>(amx_->stp);
}

bool AMXStackFrame::IsCodeAddr(ucell address) const {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx_->base);
	return address < static_cast<ucell>(hdr->dat - hdr->cod);
}

inline bool IsPrintableChar(char c) {
	return (c >= 32 && c <= 126);
}

inline char IsPrintableChar(cell c) {
	return IsPrintableChar(static_cast<char>(c & 0xFF));
}

std::string AMXStackFrame::GetPackedAMXString(cell *string, std::size_t size) const {
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

std::string AMXStackFrame::GetUnpackedAMXString(cell *string, std::size_t size) const {
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

	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx_->base);

	if (!IsDataAddr(address)) {
		return result;
	}

	cell *cstr = reinterpret_cast<cell*>(amx_->base + hdr->dat + address);

	if (size == 0) {
		// Size is unknown - copy up to the end of data.
		size = hdr->stp - address;
	}

	if (*reinterpret_cast<ucell*>(cstr) > UNPACKEDMAX) {
		result.first = GetPackedAMXString(cstr, size);
		result.second = true;
	} else {
		result.first = GetUnpackedAMXString(cstr, size);
		result.second = false;
	}

	return result;
}

AMXStackTrace::AMXStackTrace(AMX *amx, const AMXDebugInfo *debugInfo)
	: frame_(amx, amx->frm, debugInfo)
{
}

AMXStackTrace::AMXStackTrace(AMX *amx, ucell frameAddr, const AMXDebugInfo *debugInfo)
	: frame_(amx, frameAddr, debugInfo)
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
