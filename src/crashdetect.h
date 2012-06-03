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

#ifndef CRASHDETECT_H
#define CRASHDETECT_H

#include <map>
#include <stack>
#include <string>

#include "amxdebuginfo.h"
#include "configreader.h"

class crashdetect {
public:	
	static crashdetect *GetInstance(AMX *amx);
	static void DestroyInstance(AMX *amx);

	static void Crash();
	static void RuntimeError(AMX *amx, cell index, int error);
	static void Interrupt();
	static void ExitOnError();	

	int HandleAmxCallback(cell index, cell *result, cell *params);
	int HandleAmxExec(cell *retval, int index);

	void HandleCrash();
	void HandleRuntimeError(int index, int error);
	void HandleInterrupt();

	static void PrintAmxBacktrace();
	static void PrintNativeBacktrace(int framesToSkip = 0);

	static void logprintf(const char *format, ...);

private:
	explicit crashdetect(AMX *amx);

	AMX         *amx_;
	AMX_HEADER  *amxhdr_;

	AMXDebugInfo debugInfo_;

	std::string amxPath_;
	std::string amxName_;

	AMX_CALLBACK prevCallback_;

	class NPCall {
	public:
		enum Type { NATIVE, PUBLIC };

		NPCall(Type type, AMX *amx, cell index, cell frm, cell cip)
			: type_(type), amx_(amx), index_(index), frm_(frm), cip_(cip) {}

		inline Type type() const { return type_; }
		inline AMX *amx() const { return amx_; }
		inline cell index() const { return index_; }
		inline cell frm() const { return frm_; }
		inline cell cip() const { return cip_; }

	private:
		Type type_;
		AMX  *amx_;
		cell frm_;
		cell cip_;
		cell index_;
	};

	static std::stack<NPCall> npCalls_;
	static bool errorCaught_;
	static ConfigReader serverCfg;

	typedef std::map<AMX*, crashdetect*> InstanceMap;
	static InstanceMap instances_;
};

#endif // !CRASHDETECT_H
