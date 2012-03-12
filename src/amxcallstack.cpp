// Copyright (c) 2011 Zeex
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>

#include "amxcallstack.h"
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

static inline bool CompareArguments(const AMXDebugInfo::Symbol &left, const AMXDebugInfo::Symbol &right) {
	return left.GetAddress() < right.GetAddress();
}

class AddressBelongsToFunction : public std::unary_function<const AMXDebugInfo::Symbol&, bool> {
public:
	AddressBelongsToFunction(ucell address) 
		: address_(address) {}
	bool operator()(const AMXDebugInfo::Symbol &symbol) const {
		return symbol.IsFunction() &&
			address_ >= symbol.GetCodeStartAddress() && address_ <= symbol.GetCodeEndAddress();
	}
private:
	ucell address_;
};

class IsFunction : public std::unary_function<const AMXDebugInfo::Symbol&, bool> {
public:
	IsFunction(ucell address) 
		: address_(address) {}
	bool operator()(const AMXDebugInfo::Symbol &symbol) const {
		return symbol.IsFunction();
	}
private:
	ucell address_;
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

static inline bool IsAddrOnStack(ucell address, AMX *amx) {
	return (static_cast<cell>(address) >= amx->hlw 
	     && static_cast<cell>(address) <  amx->stp);
}

static inline bool IsInCode(ucell address, AMX *amx) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	return static_cast<cell>(address) < hdr->dat - hdr->cod;
}

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frameAddr, const AMXDebugInfo &debugInfo)
	: frameAddr_(frameAddr), retAddr_(0), funAddr_(0)
{
	if (IsAddrOnStack(frameAddr_, amx)) {
		AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

		ucell data = reinterpret_cast<ucell>(amx->base + hdr->dat);
		ucell code = reinterpret_cast<ucell>(amx->base) + hdr->cod;

		retAddr_ = *(reinterpret_cast<ucell*>(data + frameAddr_) + 1);
		Init(amx, debugInfo);
	}
}

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frameAddr, ucell retAddr, const AMXDebugInfo &debugInfo)
	: frameAddr_(frameAddr), retAddr_(retAddr), funAddr_(0)
{
	Init(amx, debugInfo);
}

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frameAddr, ucell retAddr, ucell funAddr, const AMXDebugInfo &debugInfo)
	: frameAddr_(frameAddr), retAddr_(retAddr), funAddr_(funAddr)
{
	Init(amx, debugInfo);
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

void AMXStackFrame::Init(AMX *amx, const AMXDebugInfo &debugInfo) {
	std::stringstream stream;

	AMXDebugInfo::Symbol funSymbol;
	if (debugInfo.IsLoaded()) {
		funSymbol = debugInfo.GetFunction(retAddr_);
	}

	if (funAddr_ == 0) {
		if (funSymbol) {
			funAddr_ = funSymbol.GetCodeStartAddress();
		} else {
			// Match return address against something in public table.
			if (GetPublicFunctionName(amx, retAddr_) != 0) {
				funAddr_ = retAddr_;
			}
		}
	}

	if (retAddr_ == funAddr_) {
		// The return address isn't real...
		stream << "???????? in ";
	} else {
		stream << std::hex << std::setw(8) << std::setfill('0') 
			<< retAddr_ << std::dec << " in ";
	}

	if (funSymbol) {
		if (IsPublicFunction(amx, funAddr_) && !IsMain(amx, funAddr_)) {
			stream << "public ";
		}
		std::string funTag = debugInfo.GetTagName((funSymbol).GetTag());
		if (!funTag.empty() && funTag != "_") {
			stream << funTag << ":";
		}		
		stream << debugInfo.GetFunctionName(funAddr_);		
	} else {		
		const char *name = GetPublicFunctionName(amx, funAddr_);
		if (name != 0) {
			stream << name;
		} else {
			stream << "??"; // unknown function
		}
	}

	stream << " (";

	if (funSymbol) {
		// Get function parameters.
		std::vector<AMXDebugInfo::Symbol> arguments;
		std::remove_copy_if(
			debugInfo.GetSymbols().begin(), 
			debugInfo.GetSymbols().end(), 
			std::back_inserter(arguments), 
			std::not1(IsArgumentOf(funSymbol.GetCodeStartAddress()))
		);

		// Order them by address.
		std::sort(arguments.begin(), arguments.end(), CompareArguments);

		// Build a comma-separated list of arguments and their values.
		for (std::vector<AMXDebugInfo::Symbol>::const_iterator arg = arguments.begin();
				arg != arguments.end(); ++arg) {		
			if (arg != arguments.begin()) {
				stream << ", ";
			}

			// For reference parameters, print the '&' sign in front of their tag.
			if (arg->IsReference()) {
				stream << "&";
			}		

			// Get parameter's tag .
			std::string tag = debugInfo.GetTag(arg->GetTag()).GetName() + ":";
			if (tag != "_:") {
				stream << tag;
			}

			// Get parameter name.
			stream << arg->GetName();

			cell value = arg->GetValue(amx);	
			if (arg->IsVariable()) {
				// Value arguments
				if (tag == "bool:") {
					stream << "=" << value ? "true" : "false";
				} else if (tag == "Float:") {
					stream << "=" << std::fixed << std::setprecision(5) << amx_ctof(value);
				} else {
					stream << "=" << value;
				}
			} else {
				// Reference arguments.
				std::vector<AMXDebugInfo::SymbolDim> dims = arg->GetDims();

				// Show array dimensions.
				if (arg->IsArray() || arg->IsArrayRef()) {
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

				stream << "=@0x" << std::hex << std::setw(8) << std::setfill('0') << value << std::dec;

				// If this is a string argument, get the text.
				if (arg->IsArray() || arg->IsArrayRef() 
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

		//int numArgs = static_cast<int>(arguments.size());
		//int numVarArgs = GetNumArgs(amx, frameAddr_) - numArgs;

		//if (numVarArgs > 0) {
		//	if (numArgs != 0) {
		//		stream << ". ";
		//	}
		//	stream << "... <" << numVarArgs << " variable ";
		//	if (numVarArgs == 1) {
		//		stream << "argument";
		//	} else {
		//		stream << "arguments";
		//	}
		//	stream << ">";
		//}
	}

	stream << ")";

	if (debugInfo.IsLoaded()) {
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

AMXCallStack::AMXCallStack(AMX *amx, const AMXDebugInfo &debugInfo, ucell topFrame) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	ucell data = reinterpret_cast<ucell>(amx->base + hdr->dat);

	ucell frm = topFrame;
	if (frm == 0) {
		frm = amx->frm;
	}

	while (frm < static_cast<ucell>(amx->stp)) {
		AMXStackFrame frame(amx, frm, debugInfo);		
		if (frame.GetReturnAddress() == 0) {
			break;
		}
		frames_.push_back(frame);
		frm = *reinterpret_cast<ucell*>(data + frm);
	} 
}
