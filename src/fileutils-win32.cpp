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

#include <string>
#include <vector>

#include <Windows.h>

#include "fileutils.h"

namespace fileutils {

const char kNativePathSepChar = '\\';
const char *kNativePathSepString = "\\";

const char kNativePathListSepChar = ';';
const char *kNativePathListSepString = ";";

void GetDirectoryFiles(const std::string &directory, const std::string &pattern, 
                       std::vector<std::string> &files) 
{
  std::string fileName;
  fileName.append(directory);
  fileName.append(kNativePathSepString);
  fileName.append(pattern);

  WIN32_FIND_DATA findFileData;

  HANDLE hFindFile = FindFirstFile(fileName.c_str(), &findFileData);
  if (hFindFile == INVALID_HANDLE_VALUE) {
    return;
  }

  do {
    if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        files.push_back(findFileData.cFileName);
    }
  } while (FindNextFile(hFindFile, &findFileData) != 0);

  FindClose(hFindFile);
}

std::string GetCurrentWorkingtDirectory() {
  DWORD size = GetCurrentDirectoryA(0, NULL);
  std::vector<char> buffer(size);
  GetCurrentDirectoryA(buffer.size(), &buffer[0]);
  return std::string(&buffer[0]);
}

} // namespace fileutils
