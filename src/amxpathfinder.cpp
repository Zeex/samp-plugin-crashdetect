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

#include <cstdlib>
#include <cstring>
#include <iterator>
#include <list>
#include <vector>

#include <amx/amxaux.h>

#include "amxpathfinder.h"
#include "amxscript.h"
#include "fileutils.h"

AMXPathFinder::AMXFile::AMXFile(const std::string &name)
 : amx_(reinterpret_cast<AMX*>(std::malloc(sizeof(*amx_)))),
   name_(name),
   mtime_(fileutils::GetModificationTime(name))
{
  if (amx_ != 0) {
    if (aux_LoadProgram(amx_, name.c_str(), 0) != AMX_ERR_NONE) {
      std::free(amx_);
      amx_ = 0;
    }
  }
}

AMXPathFinder::AMXFile::~AMXFile() {
  if (amx_ != 0) {
    aux_FreeProgram(amx_);
    std::free(amx_);
  }
}

AMXPathFinder::~AMXPathFinder() {
  for (StringToAMXFileMap::const_iterator mapIter = string_to_amx_file_.begin();
      mapIter != string_to_amx_file_.end(); ++mapIter)
  {
    delete mapIter->second;
  }
}

void AMXPathFinder::AddSearchPath(std::string path) {
  search_paths_.push_back(path);
}

std::string AMXPathFinder::Find(AMXScript amx) {
  // Look up in cache first.
  AMXToStringMap::const_iterator cache_iterator = amx_to_string_.find(amx);
  if (cache_iterator != amx_to_string_.end()) {
    return cache_iterator->second;
  }

  std::string result;

  // Load all .amx files in each of the current search paths (non-recursive).
  for (std::list<std::string>::const_iterator dir_iterator = search_paths_.begin();
      dir_iterator != search_paths_.end(); ++dir_iterator)
  {
    std::vector<std::string> files;
    fileutils::GetDirectoryFiles(*dir_iterator, "*.amx", files);

    for (std::vector<std::string>::iterator file_iterator = files.begin();
        file_iterator != files.end(); ++file_iterator)
    {
      std::string filename;
      filename.append(*dir_iterator);
      filename.append(fileutils::kNativePathSepString);
      filename.append(*file_iterator);

      std::time_t mtime = fileutils::GetModificationTime(filename);

      StringToAMXFileMap::iterator script_it = string_to_amx_file_.find(filename);
      if (script_it == string_to_amx_file_.end() ||
          script_it->second->mtime() < mtime) {
        if (script_it != string_to_amx_file_.end()) {
          string_to_amx_file_.erase(script_it);
        }
        AMXFile *script = new AMXFile(filename);
        if (script != 0 && script->IsLoaded()) {
          string_to_amx_file_.insert(std::make_pair(filename, script));
        }
      }
    }
  }

  for (StringToAMXFileMap::const_iterator mapIter = string_to_amx_file_.begin();
      mapIter != string_to_amx_file_.end(); ++mapIter)
  {
    const AMX *otherAmx = mapIter->second->amx();
    if (std::memcmp(amx.GetHeader(), otherAmx->base, sizeof(AMX_HEADER)) == 0) {
      result = mapIter->first;
      amx_to_string_.insert(std::make_pair(amx, result));
      break;
    }
  }

  return result;
}
