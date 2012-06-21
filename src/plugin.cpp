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

#include "crashdetect.h"
#include "fileutils.h"
#include "jump-x86.h"
#include "logprintf.h"
#include "os.h"
#include "plugincommon.h"
#include "version.h"
#include "amx/amx.h"

extern "C" int AMXAPI amx_Error(AMX *amx, cell index, int error) {
	if (error != AMX_ERR_NONE) {
		crashdetect::RuntimeError(amx, index, error);
	}
	return AMX_ERR_NONE;
}

static int AMXAPI AmxCallback(AMX *amx, cell index, cell *result, cell *params) {
	return crashdetect::GetInstance(amx)->HandleAmxCallback(index, result, params);
}

static int AMXAPI AmxExec(AMX *amx, cell *retval, int index) {
	return crashdetect::GetInstance(amx)->HandleAmxExec(retval, index);
}

#if defined _MSC_VER

static int CallAmxRelease(AMX *amx, cell amx_addr, void *caller) {
	return crashdetect::GetInstance(amx)->HandleAmxRelease(amx_addr, caller);
}

static __declspec(naked) int AMXAPI AmxRelease(AMX *amx, cell amx_addr) {
	__asm push ebp
	__asm mov ebp, esp
	__asm push dword ptr [ebp + 4]
	__asm push dword ptr [ebp + 12]
	__asm push dword ptr [ebp + 8]
	__asm call CallAmxRelease
	__asm add esp, 12
	__asm pop ebp
	__asm ret
}

#elif defined __GNUC__

extern "C" int AMXAPI CallAmxRelease(AMX *amx, cell amx_addr, void *caller) {
	return crashdetect::GetInstance(amx)->HandleAmxRelease(amx_addr, caller);
}

extern "C" int AMXAPI AmxRelease(AMX *amx, cell amx_addr);

__asm__ __volatile__ (
#if defined WIN32
"_AmxRelease:\n"
#else
"AmxRelease:\n"
#endif
	"pushl %ebp;"
	"movl %esp, %ebp;"
	"pushl 4(%ebp);"
	"pushl 12(%ebp);"
	"pushl 8(%ebp);"
	#if defined WIN32
	"call _CallAmxRelease;"
	#else
	"call CallAmxRelease;"
	#endif
	"add $12, %esp;"
	"pop %ebp;"
	"ret;"
);

#else
	#error Unsupported compiler
#endif

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
	void **exports = reinterpret_cast<void**>(ppData[PLUGIN_DATA_AMX_EXPORTS]);

	void *amx_Exec_ptr = exports[PLUGIN_AMX_EXPORT_Exec];
	void *amx_Exec_sub = JumpX86::GetTargetAddress(reinterpret_cast<unsigned char*>(amx_Exec_ptr));

	if (amx_Exec_sub == 0) {
		new JumpX86(amx_Exec_ptr, (void*)AmxExec);
	} else {
		std::string module = fileutils::GetFileName(os::GetModulePath(amx_Exec_sub));
		if (!module.empty()) {
			logprintf("  Warning: Runtime error detection will not work during this run because ");
			logprintf("           %s has been loaded before crashdetect.", module.c_str());
		}
	}

	void *amx_Release_ptr = exports[PLUGIN_AMX_EXPORT_Release];
	void *amx_Release_sub = JumpX86::GetTargetAddress(reinterpret_cast<unsigned char*>(amx_Release_ptr));

	if (amx_Release_sub == 0) {
		new JumpX86(amx_Release_ptr, (void*)AmxRelease);
	}

	os::SetCrashHandler(crashdetect::Crash);
	os::SetInterruptHandler(crashdetect::Interrupt);

	logprintf("  crashdetect v"PLUGIN_VERSION_STRING" is OK.");
	return true;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
	(void)crashdetect::GetInstance(amx); // force instance creation
	amx_SetCallback(amx, AmxCallback);
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
	crashdetect::DestroyInstance(amx);
	return AMX_ERR_NONE;
}
