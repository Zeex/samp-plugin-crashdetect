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
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "amxcallstack.h"
#include "amxdebuginfo.h"

#include "amx/amx.h"
#include "amx/amxdbg.h"

static bool IsFunctionArgument(const AMXDebugInfo::Symbol &symbol, ucell functionAddress) {
	return symbol.IsLocal() && symbol.GetCodeStartAddress() == functionAddress; 
}

class IsArgumentOf : public std::unary_function<AMXDebugInfo::Symbol, bool> {
public:
	IsArgumentOf(ucell function) : function_(function) {}

	bool operator()(AMXDebugInfo::Symbol symbol) const {
		return symbol.IsLocal() && symbol.GetCodeStartAddress() == function_;
	}

private:
	ucell function_;
};

static inline bool CompareArguments(const AMXDebugInfo::Symbol &left, const AMXDebugInfo::Symbol &right) {
	return left.GetAddress() < right.GetAddress();
}

class IsFunctionAt : public std::unary_function<const AMXDebugInfo::Symbol&, bool> {
public:
	IsFunctionAt(ucell address) : address_(address) {}

	bool operator()(const AMXDebugInfo::Symbol &symbol) const {
		return symbol.IsFunction() && symbol.GetAddress() == address_;
	}

private:
	ucell address_;
};

static inline const char *GetPublicFunctionName(AMX *amx, ucell address) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

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

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frameAddress, const AMXDebugInfo &debugInfo)
	: isPublic_(false)
	, frameAddress_(frameAddress)
	, callAddress_(0)
	, functionAddress_(0)
{
	if (IsOnStack(frameAddress_, amx)) {
		AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

		ucell data = reinterpret_cast<ucell>(amx->base + hdr->dat);
		ucell code = reinterpret_cast<ucell>(amx->base) + hdr->cod;

		callAddress_ = *(reinterpret_cast<ucell*>(data + frameAddress_) + 1) - 2*sizeof(cell);
		functionAddress_ = *reinterpret_cast<ucell*>(callAddress_ + sizeof(cell) + code) - code;

		Init(amx, debugInfo);
	}	
}

AMXStackFrame::AMXStackFrame(AMX *amx, ucell frameAddress, ucell callAddress, 
		ucell functionAddress, const AMXDebugInfo &debugInfo)
	: isPublic_(false)
	, frameAddress_(frameAddress)
	, callAddress_(callAddress)
	, functionAddress_(functionAddress)
{
	Init(amx, debugInfo);
}

void AMXStackFrame::Init(AMX *amx, const AMXDebugInfo &debugInfo) {
	isPublic_ = IsPublicFunction(amx, functionAddress_);

	if (debugInfo.IsLoaded()) {
		if (callAddress_ != 0) {		
			fileName_ = debugInfo.GetFileName(callAddress_); 
			lineNumber_ = debugInfo.GetLineNumber(callAddress_);
		} else {
			// Entry point		
			fileName_ = debugInfo.GetFileName(functionAddress_);
			lineNumber_ = debugInfo.GetLineNumber(functionAddress_);
		}

		// Get function return value tag
		AMXDebugInfo::SymbolTable::const_iterator it = std::find_if(
			debugInfo.GetSymbols().begin(), 
			debugInfo.GetSymbols().end(), 
			IsFunctionAt(functionAddress_)
		);
		if (it != debugInfo.GetSymbols().end()) {
			functionResultTag_ = debugInfo.GetTagName((*it).GetTag()) + ":";
			if (functionResultTag_ == "_:") {
				functionResultTag_.clear();
			}
		}

		// Get function name
		functionName_ = debugInfo.GetFunctionName(functionAddress_);
		

		// Get parameters
		std::remove_copy_if(
			debugInfo.GetSymbols().begin(), 
			debugInfo.GetSymbols().end(), 
			std::back_inserter(arguments_), 
			std::not1(IsArgumentOf(functionAddress_))
		);

		// Order them by address
		std::sort(arguments_.begin(), arguments_.end(), CompareArguments);

		// Build a comma-separated list of arguments and their values
		std::stringstream argStream;
		for (std::vector<AMXDebugInfo::Symbol>::const_iterator arg = arguments_.begin();
				arg != arguments_.end(); ++arg) {		
			if (arg != arguments_.begin()) {
				argStream << ", ";
			}

			// For reference parameters, print the '&' sign in front of their tag
			if (arg->IsReference()) {
				argStream << "&";
			}		

			// Get parameter's tag 
			std::string tag = debugInfo.GetTag(arg->GetTag()).GetName() + ":";
			if (tag != "_:") {
				argStream << tag;
			}

			// Get parameter name
			argStream << arg->GetName();

			cell value = arg->GetValue(amx);	
			if (arg->IsVariable()) {
				if (tag == "bool:") {
					argStream << "=" << value ? "true" : "false";
				} else if (tag == "Float:") {
					argStream << "=" << std::fixed << std::setprecision(5) << amx_ctof(value);
				} else {
					argStream << "=" << value;
				}
			} else {
				if (arg->IsArray() || arg->IsArrayRef()) {
					// Get array dimensions 
					std::vector<AMXDebugInfo::SymbolDim> dims = arg->GetDims();
					for (std::size_t i = 0; i < dims.size(); ++i) {
						if (dims[i].GetSize() == 0) {
							argStream << "[]";
						} else {
							std::string tag = debugInfo.GetTagName(dims[i].GetTag()) + ":";
							if (tag == "_:") tag.clear();
							argStream << "[" << tag << dims[i].GetSize() << "]";
						}
					}
				}
				argStream << "=@0x" << std::hex << value << std::dec;
			}
		}	

		// The function prototype is of the following form:
		// [public ][tag:]name([arg1=value1[, arg2=value2, [...]]])
		if (IsPublic()) {
			functionPrototype_.append("public ");
		}
		functionPrototype_
			.append(functionResultTag_)
			.append(functionName_)
			.append("(")
			.append(argStream.str())
			.append(")");
	} else {
		// No debug info available...
		const char *name = GetPublicFunctionName(amx, functionAddress_);
		if (name != 0) {
			functionName_.assign(name);
		}
	}
}

AMXCallStack::AMXCallStack(AMX *amx, const AMXDebugInfo &debugInfo, ucell topFrame) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	ucell data = reinterpret_cast<ucell>(amx->base + hdr->dat);

	ucell frm = topFrame;
	if (frm == 0) {
		frm = amx->frm;
	}

	while (frm > static_cast<ucell>(amx->stk)) {
		AMXStackFrame frame(amx, frm, debugInfo);
		frames_.push_back(frame);
		if (!frame) {
			// Invalid frame address
			return;
		}
		frm = *reinterpret_cast<ucell*>(data + frm);
	} 

	// Correct entry point address (it's not on the stack)
	if (debugInfo.IsLoaded()) {		
		if (frames_.size() > 1) {
			ucell epAddr = debugInfo.GetFunctionStartAddress(frames_[frames_.size() - 2].GetCallAddress());
			frames_.back() = AMXStackFrame(amx, frames_.back().GetFrameAddress(), 0, epAddr, debugInfo);
		} else if (frames_.size() != 0) {
			ucell epAddr = debugInfo.GetFunctionStartAddress(amx->cip);
			frames_.back() = AMXStackFrame(amx, frames_.back().GetFrameAddress(), 0, epAddr, debugInfo);
		}
	} else {
		// Can't fetch the address
		frames_.back() = AMXStackFrame(amx, 0, 0, 0);
	}
}
