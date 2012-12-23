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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE//
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef CRASHDETECT_H
#define CRASHDETECT_H

#include <map>
#include <stack>
#include <string>

#include <amx/amx.h>

#include "amxdebuginfo.h"
#include "configreader.h"

class NPCall;

class crashdetect {
public:
	static crashdetect *GetInstance(AMX *amx);
	static void DestroyInstance(AMX *amx);

	static void SystemException(void *context);
	static void SystemInterrupt(void *context);

	int DoAmxCallback(cell index, cell *result, cell *params);
	int DoAmxExec(cell *retval, int index);
	int DoAmxRelease(cell amx_addr, void *releaser);

	void HandleException();
	void HandleInterrupt();
	void HandleExecError(int index, int error);
	void HandleReleaseError(cell address, void *releaser);

private:
	static void DieOrContinue();

	static void PrintAmxBacktrace();
	static void PrintSystemBacktrace(void *context = 0);

	static void logprintf(const char *format, ...);

private:
	explicit crashdetect(AMX *amx);

	AMX *amx_;
	AMXDebugInfo debugInfo_;

	std::string amxPath_;
	std::string amxName_;

	AMX_CALLBACK prevCallback_;

	static std::stack<NPCall*> npCalls_;
	static bool errorCaught_;
	static ConfigReader serverCfg;

	typedef std::map<AMX*, crashdetect*> InstanceMap;
	static InstanceMap instances_;
};

#endif // !CRASHDETECT_H
