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
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <amx/amx.h>

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

unsigned char *GetDataPtr(AMX *amx) {
	unsigned char *data = amx->data;
	if (data == 0) {
		AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
		data = amx->base + hdr->dat;
	}
	return data;
}

unsigned char *GetCodePtr(AMX *amx) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
	unsigned char *code = amx->base + hdr->cod;
	return code;
}

void PushStack(AMX *amx, cell value) {
	amx->stk -= sizeof(cell);
	*reinterpret_cast<cell*>(GetDataPtr(amx) + amx->stk) = value;
}

cell PopStack(AMX *amx) {
	cell value = *reinterpret_cast<cell*>(GetDataPtr(amx) + amx->stk);
	amx->stk += sizeof(cell);
	return value;
}

void PopStack(AMX *amx, int ncells) {
	amx->stk += sizeof(cell) * ncells;
}

} // namespace amxutils
