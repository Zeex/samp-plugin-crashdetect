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

#include "fileutils.h"

#if defined WIN32
	#include <windows.h>
	#include <sys/types.h>
	#include <sys/stat.h>
#else
	#include <dirent.h>
	#include <fnmatch.h>
	#include <sys/stat.h>
#endif

namespace fileutils {

std::string GetFileName(const std::string &path) {
	std::string::size_type lastSep = path.find_last_of("/\\");
	if (lastSep != std::string::npos) {
		return path.substr(lastSep + 1);
	}
	return path;
}

std::string GetExtenstion(const std::string &path) {
	std::string ext;
	std::string::size_type period = path.rfind('.');
	if (period != std::string::npos) {
		ext = path.substr(period + 1);
	} 
	return ext;
}

std::time_t GetModificationTime(const std::string &path) {
	struct stat attrib;
	if (stat(path.c_str(), &attrib) == 0) {
		return attrib.st_mtime;
	}
	return 0;
}

void GetDirectoryFiles(const std::string &directory, const std::string &pattern, std::vector<std::string> &files)
{
#if defined WIN32
	WIN32_FIND_DATA findFileData;
	HANDLE hFindFile = FindFirstFile((directory + "\\" + pattern).c_str(), &findFileData);
	if (hFindFile != INVALID_HANDLE_VALUE) {
		do {
			if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				files.push_back(directory + "\\" + findFileData.cFileName);
			}
		} while (FindNextFile(hFindFile, &findFileData) != 0);
		FindClose(hFindFile);
	}
#else
	DIR *dp;
	if ((dp = opendir(directory.c_str())) != 0) {
		struct dirent *dirp;
		while ((dirp = readdir(dp)) != 0) {
			if (!fnmatch(pattern.c_str(), dirp->d_name,
							FNM_CASEFOLD | FNM_NOESCAPE | FNM_PERIOD)) {
				files.push_back(directory + "/" + dirp->d_name);
			}
		}
		closedir(dp);
	}
#endif
}

} // namespace fileutils
