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

#include <cstdlib>
#include <cstring>
#include <iterator>
#include <list>
#include <vector>

#include "amxpathfinder.h"
#include "fileutils.h"

#include "amx/amx.h"
#include "amx/amxaux.h"

AMXPathFinder::AMXFile::AMXFile(const std::string &name)
	: name_(name)
	, mtime_(fileutils::GetModificationTime(name))
	, amx_(reinterpret_cast<AMX*>(std::malloc(sizeof(*amx_))))
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
	for (StringToAMXFileMap::const_iterator mapIter = stringToAMXFile_.begin(); 
			mapIter != stringToAMXFile_.end(); ++mapIter) 
	{
		delete mapIter->second;
	}
}

void AMXPathFinder::AddSearchPath(const std::string &path) {
	searchPaths_.push_back(path);
}

std::string AMXPathFinder::FindAMX(AMX *amx) {
	// Look up in cache first 
	AMXToStringMap::const_iterator it = amxToString_.find(amx);
	if (it != amxToString_.end()) {
		return it->second;
	} 

	std::string result;

	// Load all .amx files in each of the current search paths (non-recursively)
	for (std::list<std::string>::const_iterator dirIter = searchPaths_.begin(); 
			dirIter != searchPaths_.end(); ++dirIter) 
	{
		std::vector<std::string> files;
		fileutils::GetDirectoryFiles(*dirIter, "*.amx", files);

		for (std::vector<std::string>::iterator fileIter = files.begin(); 
				fileIter != files.end(); ++fileIter) 
		{
			std::string filename = *dirIter + fileutils::kNativePathSep + *fileIter;
			std::time_t last_write = fileutils::GetModificationTime(filename);

			StringToAMXFileMap::iterator script_it = stringToAMXFile_.find(filename);				

			if (script_it == stringToAMXFile_.end() || 
					script_it->second->GetModificationTime() < last_write) {
				if (script_it != stringToAMXFile_.end()) {
					stringToAMXFile_.erase(script_it);
				}
				AMXFile *script = new AMXFile(filename);
				if (script != 0 && script->IsLoaded()) {
					stringToAMXFile_.insert(std::make_pair(filename, script));
				}
			}
		}
	}	

	for (StringToAMXFileMap::const_iterator mapIter = stringToAMXFile_.begin(); 
			mapIter != stringToAMXFile_.end(); ++mapIter) 
	{
		const AMX *otherAmx = mapIter->second->GetAmx();
		if (std::memcmp(amx->base, otherAmx->base, sizeof(AMX_HEADER)) == 0) {
			result = mapIter->first;
			amxToString_.insert(std::make_pair(amx, result));
			break;
		}
	}

	return result;
}
