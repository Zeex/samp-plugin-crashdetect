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

#ifndef NPCALL_H
#define NPCALL_H

#include "amx.h"

class NPCall {
public:
	enum Type { NATIVE, PUBLIC };

	NPCall(Type type, AMX *amx, cell index);
	NPCall(Type type, AMX *amx, cell index, cell frm, cell cip);

	static NPCall Public(AMX *amx, cell index);
	static NPCall Native(AMX *amx, cell index);

	inline Type type() const { return type_; }
	inline AMX *amx() const { return amx_; }
	inline cell index() const { return index_; }
	inline cell frm() const { return frm_; }
	inline cell cip() const { return cip_; }

	inline bool IsPublic() const { return type_ == PUBLIC; }
	inline bool IsNative() const { return type_ == NATIVE; }

private:
	Type type_;
	AMX  *amx_;
	cell frm_;
	cell cip_;
	cell index_;
};

#endif // !NPCALL_H
