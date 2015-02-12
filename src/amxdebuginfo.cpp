// Copyright (c) 2011-2015 Zeex
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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "amxdebuginfo.h"

std::vector<AMXDebugSymbolDim> AMXDebugSymbol::GetDims() const {
  std::vector<AMXDebugSymbolDim> dims;
  if ((IsArray() || IsArrayRef()) && GetNumDims() > 0) {
    const char *dimPtr = symbol_->name + std::strlen(symbol_->name) + 1;
    for (int i = 0; i < GetNumDims(); ++i) {
      SymbolDim dim(reinterpret_cast<const AMX_DBG_SYMDIM*>(dimPtr) + i);
      dims.push_back(dim);
    }
  }
  return dims;
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
  for (LineTable::const_reverse_iterator it = lines.crbegin();
       it != lines.crend(); ++it) {
    if  (it->GetAddress() <= address) {
      line = *it;
      break;
    }
  }
  return line;
}

AMXDebugFile AMXDebugInfo::GetFile(cell address) const {
  File file;
  FileTable files = GetFiles();
  for (FileTable::const_reverse_iterator it = files.crbegin();
       it != files.crend(); ++it) {
    if  (it->GetAddress() <= address) {
      file = *it;
      break;
    }
  }
  return file;
}

static bool IsBuggedForward(const AMX_DBG_SYMBOL *symbol) {
  // There seems to be a bug in Pawn compiler 3.2.3664 that adds
  // forwarded publics to symbol table even if they are not implemented.
  // Luckily it "works" only for those publics that start with '@'.
  return (symbol->name[0] == '@');
}

AMXDebugSymbol AMXDebugInfo::GetFunction(cell address) const {
  Symbol function;
  SymbolTable symbols = GetSymbols();
  for (SymbolTable::const_iterator it = symbols.begin();
       it != symbols.end(); ++it) {
    if (!it->IsFunction())
      continue;
    if (it->GetCodeStart() > address || it->GetCodeEnd() <= address)
      continue;
    if (IsBuggedForward(it->GetPOD())) 
      continue;
    function = *it;
    break;
  }
  return function;
}

AMXDebugSymbol AMXDebugInfo::GetExactFunction(cell address) const {
  Symbol function;
  SymbolTable symbols = GetSymbols();
  for (SymbolTable::const_iterator it = symbols.begin();
       it != symbols.end(); ++it) {
    if (!it->IsFunction())
      continue;
    if (IsBuggedForward(it->GetPOD())) 
      continue;
    if (it->GetCodeStart() == address) {
      function = *it;
      break;
    }
  }
  return function;
}

AMXDebugTag AMXDebugInfo::GetTag(int32_t tag_id) const {
  Tag tag;
  TagTable tags = GetTags();
  for (TagTable::const_iterator it = tags.begin();
       it != tags.end(); ++it) {
    if (it->GetID() == tag_id) {
      tag = *it;
      break;
    }
  }
  return tag;
}

AMXDebugAutomaton AMXDebugInfo::GetAutomaton(cell address) const {
  Automaton automaton;
  AutomatonTable automata = GetAutomata();
  for (AutomatonTable::const_iterator it = automata.begin();
       it != automata.end(); ++it) {
    if (it->GetAddress() == address) {
      automaton = *it;
      break;
    }
  }
  return automaton;
}

AMXDebugState AMXDebugInfo::GetState(int16_t automaton_id, int16_t state_id) const {
  State state;
  StateTable states = GetStates();
  for (StateTable::const_iterator it = states.begin();
       it != states.end(); ++it) {
    if (it->GetAutomaton() == automaton_id && it->GetID() == state_id) {
      state = *it;
      break;
    }
  }
  return state;
}

int32_t AMXDebugInfo::GetLineNumber(cell address) const {
  Line line = GetLine(address);
  if (line) {
    return line.GetNumber();
  }
  return -1;
}

std::string AMXDebugInfo::GetFileName(cell address) const {
  std::string name;
  File file = GetFile(address);
  if (file) {
    name = file.GetName();
  }
  return name;
}

std::string AMXDebugInfo::GetFunctionName(cell address) const {
  std::string name;
  Symbol function = GetFunction(address);
  if (function) {
    name = function.GetName();
  }
  return name;
}

std::string AMXDebugInfo::GetTagName(int32_t tag_id) const {
  std::string name;
  Tag tag = GetTag(tag_id);
  if (tag) {
    name = tag.GetName();
  }
  return name;
}

cell AMXDebugInfo::GetFunctionAddress(const std::string &func,
                               const std::string &file) const {
  ucell address;
  dbg_GetFunctionAddress(amxdbg_, func.c_str(), file.c_str(), &address);
  return static_cast<cell>(address);
}

cell AMXDebugInfo::GetLineAddress(long line, const std::string &file) const {
  ucell address;
  dbg_GetLineAddress(amxdbg_, line, file.c_str(), &address);
  return static_cast<cell>(address);
}

// static
bool AMXDebugInfo::IsPresent(AMX *amx) {
  uint16_t flags;
  amx_Flags(amx, &flags);
  return ((flags & AMX_FLAG_DEBUG) != 0);
}
