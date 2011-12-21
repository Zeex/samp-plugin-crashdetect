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

#ifndef AMXPATHFINDER_H
#define AMXPATHFINDER_H

#include <list>
#include <map>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/filesystem/path.hpp>

#include "amx/amx.h"

// AMXPathFinder can search for an .amx file corresponding to a given AMX instance.
class AMXPathFinder {
public:
	// Adds directory to a set of search paths
	void AddSearchPath(boost::filesystem::path path);

	// Returns true if found, and false otherwise
	bool FindAMX(AMX *amx, boost::filesystem::path &result);

	// Same as above but returns the path as a string (which can be empty)
	std::string FindAMX(AMX *amx);

private:
	std::list<boost::filesystem::path> searchPaths_;

	class AMXFile {
	public:
		static void FreeAmx(AMX *amx);

		explicit AMXFile(const std::string &name);

		bool IsLoaded() const { return amxPtr_.get() != 0; }

		const AMX *GetAmx() const { return amxPtr_.get(); }
		const std::string &GetName() const { return name_; }
		std::time_t GetLastWriteTime() const { return last_write_; }

	private:
		boost::shared_ptr<AMX> amxPtr_;
		std::string name_;
		std::time_t last_write_;
	};

	// Cache
	std::map<std::string, AMXFile> loadedScripts_;
	std::map<AMX*, std::string> foundNames_;
};

#endif // AMXPATHFINDER_H
