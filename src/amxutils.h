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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef AMXUTILS_H
#define AMXUTILS_H

#include <amx/amx.h>

namespace amxutils {

AMX_HEADER *GetHeader(AMX *amx);

unsigned char *GetDataPtr(AMX *amx);
unsigned char *GetCodePtr(AMX *amx);

AMX_FUNCSTUBNT *GetNativeTable(AMX *amx);
AMX_FUNCSTUBNT *GetPublicTable(AMX *amx);

int GetNumNatives(AMX *amx);
int GetNumNatives(AMX_HEADER *hdr);
int GetNumPublics(AMX *amx);
int GetNumPublics(AMX_HEADER *hdr);

ucell GetNativeAddr(AMX *amx, int index);
ucell GetPublicAddr(AMX *amx, int index);

const char *GetNativeName(AMX *amx, int index);
const char *GetPublicName(AMX *amx, int index);

void PushStack(AMX *amx, cell value);
cell PopStack(AMX *amx);
void PopStack(AMX *amx, int ncells);

} // namespace amxutils

#endif // !AMXUTILS_H
