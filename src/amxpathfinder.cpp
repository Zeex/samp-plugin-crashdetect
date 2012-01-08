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
