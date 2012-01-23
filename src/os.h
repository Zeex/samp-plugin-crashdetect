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

#ifndef OS_H
#define OS_H

#include <string>

// OS abstractions
namespace os {

// Finds which module (DLL/shared library/executable) a given symbol belongs to.
std::string GetModuleNameBySymbol(void *symbol);

// Sets server crash handler.
void SetCrashHandler(void (*handler)());

// Sets SIGINT/Ctrl-C handler.
void SetInterruptHandler(void (*handler)());

} // namespace os

#endif // !OS_H