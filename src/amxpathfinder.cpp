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
// // LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cstring>
#include <iterator>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "amxpathfinder.h"

#include "amx/amx.h"
#include "amx/amxaux.h"

AMXPathFinder::AMXFile::AMXFile(const std::string &name)
	: name_(name)
	, last_write_(boost::filesystem::last_write_time(name))
	, amxPtr_(new AMX, FreeAmx)
{
	if (aux_LoadProgram(amxPtr_.get(), const_cast<char*>(name.c_str()), 0) != AMX_ERR_NONE) {
		amxPtr_.reset();
	}	
}

void AMXPathFinder::AMXFile::FreeAmx(AMX *amx) {
	aux_FreeProgram(amx);
	delete amx;
}

void AMXPathFinder::AddSearchPath(boost::filesystem::path path) {
	searchPaths_.push_back(path);
}

bool AMXPathFinder::FindAMX(AMX *amx, boost::filesystem::path &result) {
	// Look up in cache first 
	std::map<AMX*, std::string>::const_iterator it = foundNames_.find(amx);
	if (it != foundNames_.end()) {
		result = it->second;
		return true;
	} 

	// Get and load all .amx files in each of the current search paths (non-recursive)
	for (std::list<boost::filesystem::path>::const_iterator dir = searchPaths_.begin(); 
			dir != searchPaths_.end(); ++dir) {
		if (!boost::filesystem::exists(*dir) || !boost::filesystem::is_directory(*dir)) {
			continue;
		}
		for (boost::filesystem::directory_iterator file(*dir); 
				file != boost::filesystem::directory_iterator(); ++file) {
			if (!boost::filesystem::is_regular_file(file->path()) ||
					!boost::algorithm::ends_with(file->path().string(), ".amx")) {
				continue;
			}

			std::string filename = file->path().relative_path().string();
			std::time_t last_write = boost::filesystem::last_write_time(filename);

			std::map<std::string, AMXFile>::iterator script_it = loadedScripts_.find(filename);				

			if (script_it == loadedScripts_.end() || 
					script_it->second.GetLastWriteTime() < last_write) {
				if (script_it != loadedScripts_.end()) {
					loadedScripts_.erase(script_it);
				}
				AMXFile script(filename);
				if (script.IsLoaded()) {
					loadedScripts_.insert(std::make_pair(filename, script));
				}
			}
		}
	}	

	for (std::map<std::string, AMXFile>::const_iterator iterator = loadedScripts_.begin(); 
			iterator != loadedScripts_.end(); ++iterator) 
	{
		const AMX *otherAmx = iterator->second.GetAmx();
		// Compare the two headers
		if (std::memcmp(amx->base, otherAmx->base, sizeof(AMX_HEADER)) == 0) {
			result = boost::filesystem::path(iterator->first);
			foundNames_.insert(std::make_pair(amx, result.string()));
			return true;
		}
	}

	return false;
}

std::string AMXPathFinder::FindAMX(AMX *amx) {
	boost::filesystem::path path;
	FindAMX(amx, path);
	return path.string();
}
