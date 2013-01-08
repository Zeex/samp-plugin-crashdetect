// Copyright (c) 2011-2012 Zeex
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

#ifndef AMXSTACKTRACE_H
#define AMXSTACKTRACE_H

#include <iosfwd>
#include <string>
#include <vector>
#include <utility>

#include <amx/amx.h>

#include "amxdebuginfo.h"

class AMXStackFrame {
public:
	static const int kMaxString = 30;

	std::ostream &operator<<(std::ostream &os) const {
		return os << AsString();
	}

	operator bool() const {
		return frameAddr_ != 0;
	}

	AMXStackFrame(AMX *amx);
	AMXStackFrame(AMX *amx, ucell frmAddr, const AMXDebugInfo *debugInfo = 0);
	AMXStackFrame(AMX *amx, ucell frmAddr, ucell retAddr, const AMXDebugInfo *debugInfo = 0);
	AMXStackFrame(AMX *amx, ucell frmAddr, ucell retAddr, ucell funAddr, const AMXDebugInfo *debugInfo = 0);

	virtual ~AMXStackFrame();

	AMX *GetAMX() { return amx_; }
	const AMXDebugInfo *GetDebugInfo() const { return debugInfo_; }

	bool HasDebugInfo() const {
		return debugInfo_ != 0 && debugInfo_->IsLoaded();
	}

	ucell GetFrameAddr() const { return frameAddr_; }
	ucell GetRetAddr() const { return retAddr_; }
	ucell GetFuncAddr() const { return funcAddr_; }

	cell GetArgValue(int index) const;
	int GetNumArgs() const;

	AMXStackFrame GetNextFrame() const;

	// Converts to a single-line human-friendly string (good for stack traces).
	virtual std::string AsString() const;

protected: // utility methods
	unsigned char *GetDataPtr() const;
	unsigned char *GetCodePtr() const;

	// Gets a pointer to public function name string stored in the AMX
	// (i.e. no copying involved).
	const char *GetPublicFuncName(ucell address) const;

	// Match function address against items stored in public table or main().
	// Note: IsPublicFuncAddr() returns true for main().
	bool IsPublicFuncAddr(ucell address) const;
	bool IsMainAddr(ucell address) const;

	// Determine whether an address belongs to one of the well known segments.
	bool IsStackAddr(ucell address) const;
	bool IsDataAddr(ucell address) const;
	bool IsCodeAddr(ucell address) const;

	// Extract string contents from the AMX image. All of these will terminate
	// when encounter a non-printable character.
	std::string GetPackedAMXString(cell *string, std::size_t size) const;
	std::string GetUnpackedAMXString(cell *string, std::size_t size) const;
	std::pair<std::string, bool> GetAMXString(cell address, std::size_t size) const;

	// Returns debug symbol corresponding to the called function. If debug info
	// is not present an empty symbol will be returned.
	AMXDebugSymbol GetFuncSymbol() const;

	// Returns an array of debug symbols that correspond to function arguments.
	// The arguments are ordered according to the function definition. If debug
	// info is not present an empty vector will be returned.
	void GetArgSymbols(std::vector<AMXDebugSymbol> &args) const;

private:
	void Init(ucell frameAddr, ucell retAddr = 0, ucell funcAddr = 0);

private:
	AMX *amx_;
	ucell frameAddr_;
	ucell retAddr_;
	ucell funcAddr_;
	const AMXDebugInfo *debugInfo_;
};

class AMXStackTrace {
public:
	AMXStackTrace(AMX *amx, const AMXDebugInfo *debugInfo = 0);
	AMXStackTrace(AMX *amx, ucell frameAddr = 0, const AMXDebugInfo *debugInfo = 0);

	// Move onto next frame or die.
	bool Next();

	AMXStackFrame GetFrame() const {
		return frame_;
	}

private:
	AMXStackFrame frame_;
};

#endif // !AMXSTACKTRACE_H
