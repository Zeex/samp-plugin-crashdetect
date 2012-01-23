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

#include "amx/amx.h"

namespace amxutils {

const char *GetNativeName(AMX *amx, cell index) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->natives + amx->base);
	if (index >= 0 && index < ((hdr->libraries - hdr->natives) / hdr->defsize)) {
		return reinterpret_cast<char*>(natives[index].nameofs + amx->base);
	}
	return 0;
}

AMX_NATIVE GetNativeAddress(AMX *amx, cell index) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->natives + amx->base);
	if (index >= 0 && index < ((hdr->libraries - hdr->natives) / hdr->defsize)) {
		return reinterpret_cast<AMX_NATIVE>(natives[index].address);
	}
	return 0;
}

ucell GetPublicAddress(AMX *amx, cell index) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	AMX_FUNCSTUBNT *publics = reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->publics + amx->base);
	if (index >=0 && index < ((hdr->natives - hdr->publics) / hdr->defsize)) {
		return publics[index].address;
	} else if (index == AMX_EXEC_MAIN) {
		return hdr->cip;
	}
	return 0;
}

const char *GetPublicName(AMX *amx, cell index) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	AMX_FUNCSTUBNT *publics = reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->publics + amx->base);
	if (index >=0 && index < ((hdr->natives - hdr->publics) / hdr->defsize)) {
		return reinterpret_cast<char*>(amx->base + publics[index].nameofs);
	} else if (index == AMX_EXEC_MAIN) {
		return "main";
	}
	return 0;
}

} // namespace amxutils