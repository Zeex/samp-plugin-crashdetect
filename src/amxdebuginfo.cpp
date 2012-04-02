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

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "amxdebuginfo.h"

std::vector<AMXDebugInfo::SymbolDim> AMXDebugInfo::Symbol::GetDims() const {
	std::vector<AMXDebugInfo::SymbolDim> dims;
	if ((IsArray() || IsArrayRef()) && GetNumDims() > 0) {
		const char *dimPtr = symbol_->name + std::strlen(symbol_->name) + 1;
		for (int i = 0; i < GetNumDims(); ++i) {
			dims.push_back(SymbolDim(reinterpret_cast<const AMX_DBG_SYMDIM*>(dimPtr) + i));
		}
	}
	return dims;
}

cell AMXDebugInfo::Symbol::GetValue(AMX *amx, ucell frm) const {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	unsigned char *data = reinterpret_cast<unsigned char*>(amx->base + hdr->dat);
	unsigned char *code = reinterpret_cast<unsigned char*>(amx->base + hdr->cod);

	ucell address = GetAddress();
	// Pawn Implementer's Guide:
	// The address is relative to either the code segment (cod), the data segment
	// (dat) or to the frame of the current function whose address is in the frm
	// pseudo-register.	
	if (address > static_cast<ucell>(hdr->cod)) {
		return *reinterpret_cast<cell*>(code + address);
	} else if (address > static_cast<ucell>(hdr->dat) 
			&& address < static_cast<ucell>(hdr->cod)) {
		return *reinterpret_cast<cell*>(data + address);
	} else {
		if (frm == 0) {
			frm = amx->frm;
		}
		return *reinterpret_cast<cell*>(data + frm + address);
	}	
}

AMXDebugInfo::AMXDebugInfo() {}

AMXDebugInfo::AMXDebugInfo(const std::string &filename) {
	Load(filename);
}

bool AMXDebugInfo::HasDebugInfo(AMX *amx) {
	uint16_t flags;
	amx_Flags(amx, &flags);
	return ((flags & AMX_FLAG_DEBUG) != 0);
}

void AMXDebugInfo::FreeAmxDbg(AMX_DBG *amxdbg) {
	if (amxdbg != 0) {
		dbg_FreeInfo(amxdbg);
		std::free(amxdbg);
	}
}

bool AMXDebugInfo::IsLoaded() const {
	return (amxdbgPtr_.get() != 0);
}

void AMXDebugInfo::Load(const std::string &filename) {
	std::FILE* fp = std::fopen(filename.c_str(), "rb");
	if (fp != 0) {
		AMX_DBG *amxdbg = reinterpret_cast<AMX_DBG*>(std::malloc(sizeof(AMX_DBG)));
		if (dbg_LoadInfo(amxdbg, fp) == AMX_ERR_NONE) {
			amxdbgPtr_.reset(amxdbg, FreeAmxDbg);
		}
		fclose(fp);
	}
}

void AMXDebugInfo::Free() {
	amxdbgPtr_.reset();
}

AMXDebugInfo::Line AMXDebugInfo::GetLine(ucell address) const {
	Line line;
	LineTable lines = GetLines();
	LineTable::const_iterator it = lines.begin();
	LineTable::const_iterator last = lines.begin();
	while (it != lines.end() && it->GetAddress() <= address) {
		last = it;
		++it;
		continue;
	}
	line = *last;
	++line.line_.line; 
	return line;
}

AMXDebugInfo::File AMXDebugInfo::GetFile(ucell address) const {
	File file;
	FileTable files = GetFiles();
	FileTable::const_iterator it = files.begin();
	FileTable::const_iterator last = files.begin();
	while (it != files.end() && it->GetAddress() <= address) {
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

AMXDebugInfo::Symbol AMXDebugInfo::GetFunction(ucell address) const {
	Symbol function;
	SymbolTable symbols = GetSymbols();
	for (SymbolTable::const_iterator it = symbols.begin(); it != symbols.end(); ++it) {
		if (!it->IsFunction())
			continue;
		if (it->GetCodeStartAddress() > address || it->GetCodeEndAddress() <= address)
			continue;
		if (IsBuggedForward(it->GetPOD())) 
			continue;
		function = *it;
		break;
	}
	return function;
}

AMXDebugInfo::Tag AMXDebugInfo::GetTag(int tagID) const {
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

int32_t AMXDebugInfo::GetLineNumber(ucell address) const {
	Line line = GetLine(address);
	if (line) {
		return line.GetNumber();
	}
	return 0;
}

std::string AMXDebugInfo::GetFileName(ucell address) const {
	std::string name;
	File file = GetFile(address);
	if (file) {
		name = file.GetName();
	}
	return name;
}

std::string AMXDebugInfo::GetFunctionName(ucell address) const {
	std::string name;
	Symbol function = GetFunction(address);
	if (function) {
		name = function.GetName();
	}
	return name;
}

std::string AMXDebugInfo::GetTagName(ucell address) const {
	std::string name;
	Tag tag = GetTag(address);
	if (tag) {
		name = tag.GetName();
	}
	return name;
}

ucell AMXDebugInfo::GetFunctionAddress(const std::string &functionName, const std::string &fileName) const {
	ucell functionAddress;
	dbg_GetFunctionAddress(amxdbgPtr_.get(), functionName.c_str(), fileName.c_str(), &functionAddress);
	return functionAddress;
}

ucell AMXDebugInfo::GetLineAddress(long line, const std::string &fileName) const {
	ucell lineAddress;
	dbg_GetLineAddress(amxdbgPtr_.get(), line, fileName.c_str(), &lineAddress);
	return lineAddress;
}
