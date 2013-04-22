// Copyright (c) 2012-2013 Zeex
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

#include <cstdlib>

#include <Windows.h>
#include <DbgHelp.h>

#include "stacktrace.h"
#include "stacktrace-generic.h"

// MSDN says that CaptureStackBackTrace() can't handle more than 62 frames on
// Windows Server 2003 and Windows XP.
static const int kMaxFrames = 62;
static const int kMaxSymbolNameLength = 1024;

class DbgHelp {
public:
	DbgHelp(HANDLE process = NULL);
	~DbgHelp();

	bool HaveSymInitialize() const {
		return SymInitialize_ != NULL;
	}
	BOOL SymInitialize(HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess) const {
		return SymInitialize_(hProcess, UserSearchPath, fInvadeProcess);
	}

	bool HaveSymFromAddr() const {
		return SymFromAddr_ != NULL;
	}
	BOOL SymFromAddr(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol) const {
		return SymFromAddr_(hProcess, Address, Displacement, Symbol);
	}

	bool HaveSymCleanup() const {
		return SymCleanup_ != NULL;
	}
	BOOL SymCleanup(HANDLE hProcess) const {
		return SymCleanup_(hProcess);
	}

	bool HaveSymGetOptions() const {
		return SymGetOptions_ != NULL;
	}
	DWORD SymGetOptions() const {
		return SymGetOptions_();
	}

	bool HaveSymSetOptions() const {
		return SymSetOptions_ != NULL;
	}
	BOOL SymSetOptions(DWORD SymOptions) const {
		return SymSetOptions_(SymOptions);
	}

	bool HaveSymGetModuleInfo64() const {
		return SymGetModuleInfo64_ != NULL;
	}
	BOOL SymGetModuleInfo64(HANDLE hProcess, DWORD64 dwAddr, PIMAGEHLP_MODULE64 ModuleInfo) const {
		return SymGetModuleInfo64_(hProcess, dwAddr, ModuleInfo);
	}

	bool HaveStackWalk64() const {
		return StackWalk64_ != NULL;
	}
	BOOL StackWalk64(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame, LPVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress) {
		return StackWalk64_(MachineType, hProcess, hThread, StackFrame, ContextRecord, ReadMemoryRoutine, FunctionTableAccessRoutine, GetModuleBaseRoutine, TranslateAddress);
	}

	inline bool is_loaded() const {
		return module_ != 0;
	}
	inline bool is_initialized() const {
		return initialized_ != FALSE;
	}

private:
	DbgHelp(const DbgHelp &);
	void operator=(const DbgHelp &);

	typedef BOOL (WINAPI *SymInitializeType)(HANDLE, PCSTR, BOOL);
	SymInitializeType SymInitialize_;

	typedef BOOL (WINAPI *SymFromAddrType)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
	SymFromAddrType SymFromAddr_;

	typedef BOOL (WINAPI *SymCleanupType)(HANDLE);
	SymCleanupType SymCleanup_;

	typedef DWORD (WINAPI *SymGetOptionsType)();
	SymGetOptionsType SymGetOptions_;

	typedef DWORD (WINAPI *SymSetOptionsType)(DWORD);
	SymSetOptionsType SymSetOptions_;

	typedef BOOL (WINAPI *SymGetModuleInfo64Type)(HANDLE, DWORD64, PIMAGEHLP_MODULE64);
	SymGetModuleInfo64Type SymGetModuleInfo64_;

	typedef BOOL (WINAPI *StackWalk64Type)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, LPVOID ContextRecord,
	                                       PREAD_PROCESS_MEMORY_ROUTINE64, PFUNCTION_TABLE_ACCESS_ROUTINE64,
	                                       PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64);
	StackWalk64Type StackWalk64_;

	HMODULE module_;
	HANDLE process_;
	BOOL initialized_;
};

DbgHelp::DbgHelp(HANDLE process)
	: module_(LoadLibrary("DbgHelp.dll"))
	, process_(process)
	, initialized_(FALSE)
{
	if (process_ == NULL) {
		process_ = GetCurrentProcess();
	}

	if (module_ != NULL) {
		SymInitialize_ = (SymInitializeType)GetProcAddress(module_, "SymInitialize");
		SymFromAddr_ = (SymFromAddrType)GetProcAddress(module_, "SymFromAddr");
		SymCleanup_ = (SymCleanupType)GetProcAddress(module_, "SymCleanup");
		SymGetOptions_ = (SymGetOptionsType)GetProcAddress(module_, "SymGetOptions");
		SymSetOptions_ = (SymSetOptionsType)GetProcAddress(module_, "SymSetOptions");
		SymGetModuleInfo64_ = (SymGetModuleInfo64Type)GetProcAddress(module_, "SymGetModuleInfo64");
		StackWalk64_ = (StackWalk64Type)GetProcAddress(module_, "StackWalk64");
		if (SymInitialize_ != NULL) {
			initialized_ = SymInitialize_(process_, NULL, TRUE);
		}
	}
}

DbgHelp::~DbgHelp() {
	if (module_ != NULL) {
		if (initialized_ && SymCleanup_ != NULL) {
			SymCleanup_(process_);
		}
		FreeLibrary(module_);
	}
}

StackTrace::StackTrace(void *their_context) {
	PCONTEXT context = reinterpret_cast<PCONTEXT>(their_context);
	CONTEXT current_context;
	if (their_context == NULL) {
		RtlCaptureContext(&current_context);
		context = &current_context;
	}

	HANDLE process = GetCurrentProcess();
	DbgHelp dbghelp(process);
	if (!dbghelp.is_loaded()) {
		goto fail;
	}

	STACKFRAME64 stack_frame;
	ZeroMemory(&stack_frame, sizeof(stack_frame));

	// http://stackoverflow.com/a/136942/249230
	stack_frame.AddrPC.Offset = context->Eip;
	stack_frame.AddrReturn.Offset = context->Eip;
	stack_frame.AddrPC.Mode = AddrModeFlat;
	stack_frame.AddrFrame.Offset = context->Ebp;
	stack_frame.AddrFrame.Mode = AddrModeFlat;
	stack_frame.AddrStack.Offset = context->Esp;
	stack_frame.AddrStack.Mode = AddrModeFlat;

	if (!dbghelp.HaveStackWalk64()) {
		goto fail;
	}

	SYMBOL_INFO *symbol = NULL;

	if (dbghelp.is_initialized()) {
		symbol = reinterpret_cast<SYMBOL_INFO*>(std::calloc(sizeof(*symbol) + kMaxSymbolNameLength, 1));
		symbol->SizeOfStruct = sizeof(*symbol);
		symbol->MaxNameLen = kMaxSymbolNameLength;

		if (dbghelp.HaveSymGetOptions() && dbghelp.HaveSymSetOptions()) {
			DWORD options = dbghelp.SymGetOptions();
			options |= SYMOPT_FAIL_CRITICAL_ERRORS;
			dbghelp.SymSetOptions(options);
		}
	}

	while (true) {
		DWORD64 address = stack_frame.AddrReturn.Offset;
		if (address <= 0) {
			break;
		}

		bool have_symbols = true;
		if (dbghelp.HaveSymGetModuleInfo64()) {
			IMAGEHLP_MODULE64 module;
			ZeroMemory(&module, sizeof(IMAGEHLP_MODULE64));
			module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
			if (dbghelp.SymGetModuleInfo64(process, address, &module)) {
				if (!module.GlobalSymbols) {
					have_symbols = false;
				}
			}
		}

		const char *name = "";
		if (have_symbols) {
			if (dbghelp.is_initialized() && dbghelp.HaveSymFromAddr() && symbol != NULL) {
				if (dbghelp.SymFromAddr(process, address, NULL, symbol)) {
					name = symbol->Name;
				}
			}
		}

		frames_.push_back(StackFrame(reinterpret_cast<void*>(address), name));

		BOOL result = dbghelp.StackWalk64(IMAGE_FILE_MACHINE_I386, process, GetCurrentThread(), &stack_frame,
		                                  (PVOID)context, NULL, NULL, NULL, NULL);
		if (!result) {
			break;
		}
	}

	if (symbol != NULL) {
		std::free(symbol);
	}

	return;

fail:
	frames_ = StackTraceGeneric(
		reinterpret_cast<void*>(context->Ebp),
		reinterpret_cast<void*>(context->Eip)).GetFrames();
}
