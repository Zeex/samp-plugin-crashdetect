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
#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "amxstacktrace.h"
#include "amxdebuginfo.h"

#include "amx/amx.h"
#include "amx/amxdbg.h"

static bool IsFunctionArgument(const AMXDebugInfo::Symbol &symbol, ucell functionAddress) {
	return symbol.IsLocal() && symbol.GetCodeStartAddress() == functionAddress; 
}

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

static inline const char *GetPublicFunctionName(AMX *amx, ucell address) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

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

static inline bool IsPublicFunction(AMX *amx, ucell address) {
	return GetPublicFunctionName(amx, address) != 0;
}

static inline bool IsMain(AMX *amx, ucell address) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	return static_cast<cell>(address) == hdr->cip;
}

static inline bool IsOnStack(ucell address, AMX *amx) {
	return (static_cast<cell>(address) >= amx->hlw 
	     && static_cast<cell>(address) <  amx->stp);
}

static inline bool IsInCode(ucell address, AMX *amx) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	return static_cast<cell>(address) < hdr->dat - hdr->cod;
}

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frmAddr, const AMXDebugInfo &debugInfo) 
	: frmAddr_(0), retAddr_(0), funAddr_(0)
{
	ucell retAddr = 0;

	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	ucell data = reinterpret_cast<ucell>(amx->base + hdr->dat);
	ucell code = reinterpret_cast<ucell>(amx->base) + hdr->cod;

	if (IsOnStack(frmAddr, amx)) {
		retAddr = *(reinterpret_cast<ucell*>(data + frmAddr) + 1);
	}

	Init(amx, frmAddr, retAddr, 0, debugInfo);
}

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frmAddr, ucell retAddr, const AMXDebugInfo &debugInfo) 
	: frmAddr_(0), retAddr_(0), funAddr_(0)
{
	Init(amx, frmAddr, retAddr, 0, debugInfo);
}

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frmAddr, ucell retAddr, ucell funAddr, const AMXDebugInfo &debugInfo) 
	: frmAddr_(0), retAddr_(0), funAddr_(0)
{
	Init(amx, frmAddr, retAddr, funAddr, debugInfo);
}

static inline char ToASCII(char c) {
	if (c >= 32 && c <= 126) {
		return c;
	}
	return '\0';
}

static inline char ToASCII(cell c) {
	return ToASCII(static_cast<char>(c & 0xFF));
}

static inline std::string GetPackedAMXString(AMX *amx, cell *string, std::size_t size) {
	std::string s;
	for (std::size_t i = 0; i < size; i++) {
		cell cp = string[i / sizeof(cell)] >> ((sizeof(cell) - i % sizeof(cell) - 1) * 8);
		char cu = ToASCII(cp);
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
		char c = ToASCII(string[i]);
		if (c == '\0') {
			break;
		}
		s.push_back(c);
	}
	return s;
}

static inline int GetNumArgs(AMX *amx, ucell frame) {
	if (frame > 0) {
		AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
		unsigned char *data = (amx->data != 0) ? amx->data : (amx->base + hdr->dat);
		return *reinterpret_cast<cell*>(data + frame + 2*sizeof(cell)) / sizeof(cell);
	}
	return -1;
}

static std::pair<std::string, bool> GetAMXString(AMX *amx, cell address, std::size_t size) {
	std::pair<std::string, bool> result = std::make_pair("", false);

	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	if (address < 0 || address >= hdr->hea - hdr->dat) {
		// The address is not inside the data section...
		return result;
	}

	cell *cstr = reinterpret_cast<cell*>(amx->base + hdr->dat + address);

	if (size == 0) {
		// Size is unknown, copy up to the end of data.
		size = hdr->hea - address; 
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

static cell GetArgument(AMX *amx, int index, cell frame) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	unsigned char *data = amx->data;
	if (data == 0) {
		data = amx->base + hdr->dat;
	}
	return *reinterpret_cast<cell*>(data + frame + (3 + index) * sizeof(cell));
}

static cell NextFrame(AMX *amx, cell frame) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	unsigned char *data = amx->data;
	if (data == 0) {
		data = amx->base + hdr->dat;
	}
	return *reinterpret_cast<cell*>(data + frame);
}

void AMXStackFrame::Init(AMX *amx, ucell frmAddr, ucell retAddr, ucell funAddr, const AMXDebugInfo &debugInfo) {
	if (IsOnStack(frmAddr, amx)) {
		frmAddr_ = frmAddr;
	}
	if (IsInCode(retAddr, amx)) {
		retAddr_ = retAddr;
	}
	if (IsInCode(funAddr, amx)) {
		funAddr_ = funAddr;
	}

	std::stringstream stream;

	if (debugInfo.IsLoaded()) {
		fun_ = debugInfo.GetFunction(retAddr_);
	}

	if (funAddr_ == 0) {
		if (fun_) {
			funAddr_ = fun_.GetCodeStartAddress();
		}
	}

	if (retAddr_ == 0) {
		stream << "???????? in ";
	} else {
		stream << std::hex << std::setw(8) << std::setfill('0') 
			<< retAddr_ << std::dec << " in ";
	}

	if (fun_) {
		if (IsPublicFunction(amx, funAddr_) && !IsMain(amx, funAddr_)) {
			stream << "public ";
		}
		std::string funTag = debugInfo.GetTagName((fun_).GetTag());
		if (!funTag.empty() && funTag != "_") {
			stream << funTag << ":";
		}		
		stream << debugInfo.GetFunctionName(funAddr_);		
	} else {		
		const char *name = GetPublicFunctionName(amx, funAddr_);
		if (name != 0) {
			if (!IsMain(amx, funAddr_)) {
				stream << "public ";
			}
			stream << name;
		} else {
			stream << "??"; // unknown function
		}
	}

	stream << " (";

	if (fun_ && frmAddr_ != 0) {
		// Get function arguments and sort the by address.
		std::remove_copy_if(
			debugInfo.GetSymbols().begin(), 
			debugInfo.GetSymbols().end(), 
			std::back_inserter(args_), 
			std::not1(IsArgumentOf(fun_.GetCodeStartAddress()))
		);
		std::sort(args_.begin(), args_.end());

		// Build a comma-separated list of arguments and their values.
		for (std::size_t i = 0; i < args_.size(); i++) {
			AMXDebugInfo::Symbol &arg = args_[i];

			// Argument separator.
			if (i != 0) {
				stream << ", ";
			}

			// For reference arguments print the "&" sign.
			if (arg.IsReference()) {
				stream << "&";
			}

			// Print either "tag:name" or just "name" it has no tag.
			std::string tag = debugInfo.GetTag(arg.GetTag()).GetName() + ":";
			if (tag == "_:") {
				stream << arg.GetName();
			} else {
				stream << tag << arg.GetName();
			}

			// Print argument's value depending on its type and tag.
			cell value = GetArgument(amx, i, NextFrame(amx, frmAddr_));
			if (arg.IsVariable()) {
				if (tag == "bool:") {
					// Boolean.
					stream << "=" << (value ? "true" : "false");
				} else if (tag == "Float:") {
					// Floating-point number.
					stream << "=" << std::fixed << std::setprecision(5) << amx_ctof(value);
				} else {
					// Something other...
					stream << "=" << value;
				}
			} else {
				std::vector<AMXDebugInfo::SymbolDim> dims = arg.GetDims();
				if (arg.IsArray() || arg.IsArrayRef()) {
					for (std::size_t i = 0; i < dims.size(); ++i) {
						if (dims[i].GetSize() == 0) {
							stream << "[]";
						} else {
							std::string tag = debugInfo.GetTagName(dims[i].GetTag()) + ":";
							if (tag == "_:") tag.clear();
							stream << "[" << tag << dims[i].GetSize() << "]";
						}
					}
				}

				// For arrays/references we print their AMX address.
				stream << "=@0x" << std::hex << std::setw(8) << std::setfill('0') << value << std::dec;

				// If this is a string argument, get the text.
				if ((arg.IsArray() || arg.IsArrayRef())
						&& dims.size() == 1
						&& tag == "_:"
						&& debugInfo.GetTagName(dims[0].GetTag()) == "_") 
				{
					std::pair<std::string, bool> s = GetAMXString(amx, value, dims[0].GetSize());
					stream << " ";
					if (s.second) {
						stream << "!"; // packed string
					}
					if (s.first.length() > kMaxString) {
						// The text is too long.
						s.first.replace(kMaxString, s.first.length() - kMaxString, "...");
					}
					stream << "\"" << s.first << "\"";
				}
			}
		}	

		int numArgs = static_cast<int>(args_.size());
		int numVarArgs = GetNumArgs(amx, NextFrame(amx, frmAddr_)) - numArgs;

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

	if (debugInfo.IsLoaded() && retAddr_ != 0) {
		std::string fileName = debugInfo.GetFileName(retAddr_);
		if (!fileName.empty()) {
			stream << " at " << fileName;
		}
		long line = debugInfo.GetLineNumber(retAddr_);
		if (line != 0) {
			stream << ":" << line;
		}
	}

	string_ = stream.str();
}

AMXStackTrace::AMXStackTrace(AMX *amx, const AMXDebugInfo &debugInfo, ucell topFrame) {
	ucell frm = topFrame;
	if (frm == 0) {
		frm = amx->frm;
	}

	while (frm < static_cast<ucell>(amx->stp) 
			&& frm >= static_cast<ucell>(amx->hlw)) 
	{
		AMXStackFrame frame(amx, frm, debugInfo);
		if (frame.GetReturnAddress() == 0) {
			break;
		} else {
			frames_.push_back(frame);
			frm = NextFrame(amx, frm);
		}
	} 
}
