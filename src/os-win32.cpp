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

#include "os.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

std::string os::GetModuleNameBySymbol(void *symbol) {
	char module[FILENAME_MAX] = "";

	if (symbol != 0) {
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(symbol, &mbi, sizeof(mbi));
		GetModuleFileName((HMODULE)mbi.AllocationBase, module, FILENAME_MAX);
	}
	
	return std::string(module);
}