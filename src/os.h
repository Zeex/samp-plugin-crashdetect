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

#ifndef OS_H
#define OS_H

#include <cstddef>
#include <cstdio>
#include <string>

namespace os {

// Platform-dependent directory separation character. It is usually defined as
// a back slash ('\\') on Windows and a forth slash ('/') on *nix OSes.
extern const char kDirSepChar;

const std::size_t kMaxModulePathLength = FILENAME_MAX;
const std::size_t kMaxSymbolNameLength = 256;

// GetModuleNameByAddress finds which module (executable/DLL) a given 
// address belongs to.
std::string GetModulePath(void *address, std::size_t maxLength = kMaxModulePathLength);

// SetCrashHandle sets a global exception handler on Windows and SIGSEGV
// signal handler on Linux.
void SetCrashHandler(void (*handler)());

// SetInterruptHandler sets a global Ctrl+C event handler on Windows
// and SIGINT signal handler on Linux.
void SetInterruptHandler(void (*handler)());

// GetSymbolName finds symbol name by address.
std::string GetSymbolName(void *address, std::size_t maxLength = kMaxSymbolNameLength);

// ListDirectoryFiles enumerates directory files whose name matches a specific pattern
// and calls a given function (callback) for each of them until the callback returns false
// or no more files exist.
void ListDirectoryFiles(const std::string &directory,
                        const std::string &pattern,
                        bool (*callback)(const char *filename, void *userData),
                        void *userData);

} // namespace os

#endif // !OS_H
