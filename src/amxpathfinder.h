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

#ifndef AMXPATHFINDER_H
#define AMXPATHFINDER_H

#include <ctime>
#include <list>
#include <map>
#include <string>

#include "amx/amx.h"

// AMXPathFinder can search for an .amx file corresponding to a given AMX instance.
class AMXPathFinder {
public:
	~AMXPathFinder();

	// Adds directory to a set of search paths
	void AddSearchPath(const std::string &path);

	// Same as above but returns the path as a string (which can be empty)
	std::string FindAMX(AMX *amx);

private:
	std::list<std::string> searchPaths_;

	class AMXFile {
	public:
		explicit AMXFile(const std::string &name);
		~AMXFile();

		bool IsLoaded() const { return amx_ != 0; }

		const AMX *GetAmx() const { return amx_; }
		const std::string &GetName() const { return name_; }
		std::time_t GetModificationTime() const { return mtime_; }

	private:
		// Disallow copying
		AMXFile(const AMXFile &other);
		AMXFile &operator=(const AMXFile &other);

		AMX *amx_;
		std::string name_;
		std::time_t mtime_;
	};

	typedef std::map<std::string, AMXFile*> StringToAMXFileMap;
	StringToAMXFileMap stringToAMXFile_;

	typedef std::map<AMX*, std::string> AMXToStringMap;
	AMXToStringMap amxToString_;
};

#endif // AMXPATHFINDER_H
