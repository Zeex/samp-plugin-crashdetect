// Copyright (c) 2011-2012, Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met: 
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution. 
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include <amx/amxdbg.h>

#include "amxstacktrace.h"
#include "amxdebuginfo.h"

class IsArgumentOf : public std::unary_function<AMXDebugInfo::Symbol, bool> {
public:
	IsArgumentOf(ucell function) 
		: function_(function) {}
	bool operator()(AMXDebugInfo::Symbol symbol) const {
		return symbol.IsLocal() && symbol.GetCodeStartAddress() == function_;
	}
private:
	ucell function_;
};

static inline AMX_HEADER *GetAMXHeader(AMX *amx) {
	return reinterpret_cast<AMX_HEADER*>(amx->base);
}

static unsigned char *GetAMXData(AMX *amx) {
	AMX_HEADER *hdr = GetAMXHeader(amx);
	unsigned char *data = amx->data;
	if (data == 0) {
		data = amx->base + hdr->dat;
	}
	return data;
}

static unsigned char *GetAMXCode(AMX *amx) {
	AMX_HEADER *hdr = GetAMXHeader(amx);
	return amx->base + hdr->cod;
}

static inline const char *GetPublicFuncName(AMX *amx, ucell address) {
	AMX_HEADER *hdr = GetAMXHeader(amx);

	if (address == hdr->cip) {
		return "main";
	}

	AMX_FUNCSTUBNT *publics = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->publics);
	AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->natives);

	for (AMX_FUNCSTUBNT *p = publics; p < natives; ++p) {
		if (p->address == address) {
			return reinterpret_cast<const char*>(p->nameofs + amx->base);
		}
	}

	return 0;
}

static inline bool IsPublicFuncAddr(AMX *amx, ucell address) {
	return GetPublicFuncName(amx, address) != 0;
}

static inline bool IsMainAddr(AMX *amx, ucell address) {
	AMX_HEADER *hdr = GetAMXHeader(amx);
	return static_cast<cell>(address) == hdr->cip;
}

static inline bool IsStackAddr(ucell address, AMX *amx) {
	return (address >= static_cast<ucell>(amx->hlw)
		&& address < static_cast<ucell>(amx->stp));
}

static inline bool IsDataAddr(ucell address, AMX *amx) {
	return address < static_cast<ucell>(amx->stp);
}

static inline bool IsCodeAddr(ucell address, AMX *amx) {
	AMX_HEADER *hdr = GetAMXHeader(amx);
	return address < static_cast<ucell>(hdr->dat - hdr->cod);
}

static inline char AsPrintableASCII(char c) {
	if (c >= 32 && c <= 126) {
		return c;
	}
	return '\0';
}

static inline char AsPrintableASCII(cell c) {
	return AsPrintableASCII(static_cast<char>(c & 0xFF));
}

static inline std::string GetPackedAMXString(AMX *amx, cell *string, std::size_t size) {
	std::string s;
	for (std::size_t i = 0; i < size; i++) {
		cell cp = string[i / sizeof(cell)] >> ((sizeof(cell) - i % sizeof(cell) - 1) * 8);
		char cu = AsPrintableASCII(cp);
		if (cu == '\0') {
			break;
		}
		s.push_back(cu);
	}

	return s;
}

static inline std::string GetUnpackedAMXString(AMX *amx, cell *string, std::size_t size) {
	std::string s;
	for (std::size_t i = 0; i < size; i++) {
		char c = AsPrintableASCII(string[i]);
		if (c == '\0') {
			break;
		}
		s.push_back(c);
	}
	return s;
}

static std::pair<std::string, bool> GetAMXString(AMX *amx, cell address, std::size_t size) {
	std::pair<std::string, bool> result = std::make_pair("", false);

	AMX_HEADER *hdr = GetAMXHeader(amx);

	if (!IsDataAddr(address, amx)) {
		return result;
	}

	cell *cstr = reinterpret_cast<cell*>(amx->base + hdr->dat + address);

	if (size == 0) {
		// Size is unknown - copy up to the end of data.
		size = hdr->stp - address;
	}

	if (*reinterpret_cast<ucell*>(cstr) > UNPACKEDMAX) {
		result.first = GetPackedAMXString(amx, cstr, size);
		result.second = true;
	} else {
		result.first = GetUnpackedAMXString(amx, cstr, size);
		result.second = false;
	}

	return result;
}

AMXStackFrame::AMXStackFrame(AMX *amx)
	: amx_(amx), frameAddr_(0), retAddr_(0), funcAddr_(0)
{
}

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frameAddr, const AMXDebugInfo *debugInfo)
	: amx_(amx), frameAddr_(0), retAddr_(0), funcAddr_(0), debugInfo_(debugInfo)
{
	ucell data = reinterpret_cast<ucell>(GetAMXData(amx_));
	ucell code = reinterpret_cast<ucell>(GetAMXCode(amx_));

	if (IsStackAddr(frameAddr, amx_)) {
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
	if (IsStackAddr(frameAddr, amx_)) {
		frameAddr_ = frameAddr;
	}
	if (IsCodeAddr(retAddr, amx_)) {
		retAddr_ = retAddr;
	}
	if (IsCodeAddr(funcAddr, amx_)) {
		funcAddr_ = funcAddr;
	}

	if (funcAddr_ == 0) {
		AMXDebugInfo::Symbol func = GetFuncSymbol();
		if (func) {
			funcAddr_ = func.GetCodeStartAddress();
		}
	}
}

AMXStackFrame::~AMXStackFrame() {
}

cell AMXStackFrame::GetArgValue(int index) const {
	unsigned char *data = GetAMXData(amx_);
	return *reinterpret_cast<cell*>(data + frameAddr_ + (3 + index) * sizeof(cell));
}

int AMXStackFrame::GetNumArgs() const {
	if (frameAddr_ > 0) {
		unsigned char *data = GetAMXData(amx_);
		return *reinterpret_cast<cell*>(data + frameAddr_ + 2*sizeof(cell)) / sizeof(cell);
	}
	return -1;
}

AMXStackFrame AMXStackFrame::GetNextFrame() const {
	if (frameAddr_ != 0 && retAddr_ != 0) {
		unsigned char *data = GetAMXData(amx_);
		return AMXStackFrame(amx_, *reinterpret_cast<cell*>(data + frameAddr_), debugInfo_);
	} else {
		return AMXStackFrame(amx_);
	}
}

AMXDebugInfo::Symbol AMXStackFrame::GetFuncSymbol() const {
	AMXDebugInfo::Symbol func;
	if (HasDebugInfo()) {
		func = debugInfo_->GetFunction(retAddr_);
	}
	return func;
}

void AMXStackFrame::GetArgSymbols(std::vector<AMXDebugInfo::Symbol> &args) const {
	if (HasDebugInfo()) {
		AMXDebugInfo::Symbol func = debugInfo_->GetFunction(retAddr_);

		std::remove_copy_if(
			debugInfo_->GetSymbols().begin(),
			debugInfo_->GetSymbols().end(),
			std::back_inserter(args),
			std::not1(IsArgumentOf(func.GetCodeStartAddress()))
		);
		std::sort(args.begin(), args.end());
	}
}

std::string AMXStackFrame::AsString() const {
	std::stringstream stream;

	AMXDebugInfo::Symbol func = GetFuncSymbol();

	if (retAddr_ == 0) {
		stream << "???????? in ";
	} else {
		stream << std::hex << std::setw(8) << std::setfill('0') 
			<< retAddr_ << std::dec << " in ";
	}

	if (func) {
		if (IsPublicFuncAddr(amx_, funcAddr_) && !IsMainAddr(amx_, funcAddr_)) {
			stream << "public ";
		}
		std::string funTag = debugInfo_->GetTagName((func).GetTag());
		if (!funTag.empty() && funTag != "_") {
			stream << funTag << ":";
		}		
		stream << debugInfo_->GetFunctionName(funcAddr_);
	} else {		
		const char *name = GetPublicFuncName(amx_, funcAddr_);
		if (name != 0) {
			if (!IsMainAddr(amx_, funcAddr_)) {
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

		std::vector<AMXDebugInfo::Symbol> args;
		GetArgSymbols(args);

		// Build a comma-separated list of arguments and their values.
		for (std::size_t i = 0; i < args.size(); i++) {
			AMXDebugInfo::Symbol &arg = args[i];

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
				std::vector<AMXDebugInfo::SymbolDim> dims = arg.GetDims();
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
					std::pair<std::string, bool> s = GetAMXString(amx_, value, dims[0].GetSize());
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
		long line = debugInfo_->GetLineNumber(retAddr_);
		if (line != 0) {
			stream << ":" << line;
		}
	}

	return stream.str();
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
