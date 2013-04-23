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

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <amx/amx.h>
#include <amx/amxdbg.h>

#include "amxdebuginfo.h"

std::vector<AMXDebugSymbolDim> AMXDebugSymbol::GetDims() const {
	std::vector<AMXDebugSymbolDim> dims;
	if ((IsArray() || IsArrayRef()) && GetNumDims() > 0) {
		const char *dimPtr = symbol_->name + std::strlen(symbol_->name) + 1;
		for (int i = 0; i < GetNumDims(); ++i) {
			dims.push_back(SymbolDim(reinterpret_cast<const AMX_DBG_SYMDIM*>(dimPtr) + i));
		}
	}
	return dims;
}

cell AMXDebugSymbol::GetValue(AMX *amx, cell frm) const {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	unsigned char *data = reinterpret_cast<unsigned char*>(amx->base + hdr->dat);
	unsigned char *code = reinterpret_cast<unsigned char*>(amx->base + hdr->cod);

	cell address = GetAddr();
	// Pawn Implementer's Guide:
	// The address is relative to either the code segment (cod), the data segment
	// (dat) or to the frame of the current function whose address is in the frm
	// pseudo-register.	
	if (address > hdr->cod) {
		return *reinterpret_cast<cell*>(code + address);
	} else if (address > hdr->dat && address < hdr->cod) {
		return *reinterpret_cast<cell*>(data + address);
	} else {
		if (frm == 0) {
			frm = amx->frm;
		}
		return *reinterpret_cast<cell*>(data + frm + address);
	}	
}

AMXDebugInfo::AMXDebugInfo()
	: amxdbg_(0)
{
}

AMXDebugInfo::AMXDebugInfo(const std::string &filename)
	: amxdbg_(0)
{
	Load(filename);
}

AMXDebugInfo::~AMXDebugInfo() {
	Free();
}

bool AMXDebugInfo::HasDebugInfo(AMX *amx) {
	uint16_t flags;
	amx_Flags(amx, &flags);
	return ((flags & AMX_FLAG_DEBUG) != 0);
}

bool AMXDebugInfo::IsLoaded() const {
	return (amxdbg_ != 0);
}

void AMXDebugInfo::Load(const std::string &filename) {
	std::FILE* fp = std::fopen(filename.c_str(), "rb");
	if (fp != 0) {
		AMX_DBG amxdbg;
		if (dbg_LoadInfo(&amxdbg, fp) == AMX_ERR_NONE) {
			amxdbg_ = new AMX_DBG(amxdbg);
		}
		fclose(fp);
	}
}

void AMXDebugInfo::Free() {
	if (amxdbg_ != 0) {
		dbg_FreeInfo(amxdbg_);
		delete amxdbg_;
	}
}

AMXDebugLine AMXDebugInfo::GetLine(cell address) const {
	Line line;
	LineTable lines = GetLines();
	LineTable::const_iterator it = lines.begin();
	LineTable::const_iterator last = lines.begin();
	while (it != lines.end() && it->GetAddr() <= address) {
		last = it;
		++it;
		continue;
	}
	line = *last;
	++line.line_.line; 
	return line;
}

AMXDebugFile AMXDebugInfo::GetFile(cell address) const {
	File file;
	FileTable files = GetFiles();
	FileTable::const_iterator it = files.begin();
	FileTable::const_iterator last = files.begin();
	while (it != files.end() && it->GetAddr() <= address) {
		last = it;
		++it;
		continue;
	}
	file = *last;
	return file;
}

static bool IsBuggedForward(const AMX_DBG_SYMBOL *symbol) {
	// There seems to be a bug in Pawn compiler 3.2.3664 that adds
	// forwarded publics to symbol table even if they are not implemented.
	// Luckily it "works" only for those publics that start with '@'.
	return (symbol->name[0] == '@');
}

AMXDebugSymbol AMXDebugInfo::GetFunc(cell address) const {
	Symbol function;
	SymbolTable symbols = GetSymbols();
	for (SymbolTable::const_iterator it = symbols.begin(); it != symbols.end(); ++it) {
		if (!it->IsFunction())
			continue;
		if (it->GetCodeStartAddr() > address || it->GetCodeEndAddr() <= address)
			continue;
		if (IsBuggedForward(it->GetPOD())) 
			continue;
		function = *it;
		break;
	}
	return function;
}

AMXDebugTag AMXDebugInfo::GetTag(int tagID) const {
	Tag tag;
	TagTable tags = GetTags();
	TagTable::const_iterator it = tags.begin();
	while (it != tags.end() && it->GetID() != tagID) {
		++it;
		continue;
	}
	if (it != tags.end()) {
		tag = *it;
	}
	return tag;
}

int32_t AMXDebugInfo::GetLineNo(cell address) const {
	Line line = GetLine(address);
	if (line) {
		return line.GetNo();
	}
	return 0;
}

std::string AMXDebugInfo::GetFileName(cell address) const {
	std::string name;
	File file = GetFile(address);
	if (file) {
		name = file.GetName();
	}
	return name;
}

std::string AMXDebugInfo::GetFuncName(cell address) const {
	std::string name;
	Symbol function = GetFunc(address);
	if (function) {
		name = function.GetName();
	}
	return name;
}

std::string AMXDebugInfo::GetTagName(cell address) const {
	std::string name;
	Tag tag = GetTag(address);
	if (tag) {
		name = tag.GetName();
	}
	return name;
}

cell AMXDebugInfo::GetFuncAddr(const std::string &functionName, const std::string &fileName) const {
	ucell address;
	dbg_GetFunctionAddress(amxdbg_, functionName.c_str(), fileName.c_str(), &address);
	return static_cast<cell>(address);
}

cell AMXDebugInfo::GetLineAddr(long line, const std::string &fileName) const {
	ucell address;
	dbg_GetLineAddress(amxdbg_, line, fileName.c_str(), &address);
	return static_cast<cell>(address);
}

// static
bool AMXDebugInfo::IsPresent(AMX *amx) {
	uint16_t flags;
	amx_Flags(amx, &flags);
	return ((flags & AMX_FLAG_DEBUG) != 0);
}
