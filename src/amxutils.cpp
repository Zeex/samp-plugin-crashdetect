// Copyright (c) 2012 Zeex
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE//
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <amx/amx.h>

namespace amxutils {

AMX_HEADER *GetHeader(AMX *amx) {
	return reinterpret_cast<AMX_HEADER*>(amx->base);
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


AMX_FUNCSTUBNT *GetNativeTable(AMX *amx) {
	AMX_HEADER *hdr = GetHeader(amx);
	return reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->natives + amx->base);
}

AMX_FUNCSTUBNT *GetPublicTable(AMX *amx) {
	AMX_HEADER *hdr = GetHeader(amx);
	return reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->publics + amx->base);
}

int GetNumNatives(AMX_HEADER *hdr) {
	return (hdr->libraries - hdr->natives) / hdr->defsize;
}

int GetNumNatives(AMX *amx) {
	AMX_HEADER *hdr = GetHeader(amx);
	return GetNumNatives(hdr);
}

int GetNumPublics(AMX_HEADER *hdr) {
	return (hdr->natives - hdr->publics) / hdr->defsize;
}

int GetNumPublics(AMX *amx) {
	AMX_HEADER *hdr = GetHeader(amx);
	return GetNumPublics(hdr);
}

ucell GetNativeAddr(AMX *amx, int index) {
	int n = GetNumNatives(amx);
	AMX_FUNCSTUBNT *natives = GetNativeTable(amx);
	if (index >= 0 && index < n) {
		return natives[index].address;
	}
	return 0;
}

ucell GetPublicAddr(AMX *amx, int index) {
	int n = GetNumPublics(amx);
	AMX_FUNCSTUBNT *publics = GetPublicTable(amx);
	if (index >=0 && index < n) {
		return publics[index].address;
	} else if (index == AMX_EXEC_MAIN) {
		AMX_HEADER *hdr = GetHeader(amx);
		return hdr->cip;
	}
	return 0;
}

const char *GetNativeName(AMX *amx, int index) {
	int n = GetNumNatives(amx);
	AMX_FUNCSTUBNT *natives = GetNativeTable(amx);
	if (index >= 0 && index < n) {
		return reinterpret_cast<char*>(natives[index].nameofs + amx->base);
	}
	return 0;
}

const char *GetPublicName(AMX *amx, int index) {
	int n = GetNumPublics(amx);
	AMX_FUNCSTUBNT *publics = GetPublicTable(amx);
	if (index >=0 && index < n) {
		return reinterpret_cast<char*>(amx->base + publics[index].nameofs);
	} else if (index == AMX_EXEC_MAIN) {
		return "main";
	}
	return 0;
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
