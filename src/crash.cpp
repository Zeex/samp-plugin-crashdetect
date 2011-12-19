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

#include "crash.h"

Crash::Handler Crash::handler_;
bool Crash::miniDumpEnabled_ = false;

#ifdef _WIN32

#include <Windows.h>
#include <DbgHelp.h>
#include <stdio.h>
#include <tchar.h>

static void WriteDump(PEXCEPTION_POINTERS exceptionPointers) {
	HMODULE DbgHelpDLL= LoadLibrary("DbgHelp.dll");
	if (DbgHelpDLL == NULL)
		return;

	typedef BOOL (WINAPI *MiniDumpWriteDumpType)(
			HANDLE hProcess,
			DWORD ProcessId,
			HANDLE hFile,
			MINIDUMP_TYPE DumpType,
			PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
			PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
			PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

	MiniDumpWriteDumpType MiniDumpWriteDump =
		(MiniDumpWriteDumpType)GetProcAddress(DbgHelpDLL, "MiniDumpWriteDump");
	if (MiniDumpWriteDump == NULL)
		return;

	TCHAR outFileName[FILENAME_MAX];
	GetModuleFileName(GetModuleHandle(NULL), outFileName, FILENAME_MAX);

	// Replace .exe with .dmp
	size_t length = _tcslen(outFileName);
	outFileName[length-3] = _T('d');
	outFileName[length-2] = _T('m');
	outFileName[length-1] = _T('p');

	HANDLE outFile = CreateFile(outFileName, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (outFile == INVALID_HANDLE_VALUE)
		return;

	MINIDUMP_EXCEPTION_INFORMATION mdExceptionInfo;
	mdExceptionInfo.ThreadId = GetCurrentThreadId();
	mdExceptionInfo.ExceptionPointers = exceptionPointers;
	mdExceptionInfo.ClientPointers = TRUE;

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), outFile,
		MiniDumpWithDataSegs, &mdExceptionInfo, NULL, NULL);

	CloseHandle(outFile);
}

static LPTOP_LEVEL_EXCEPTION_FILTER previousExceptionFilter;

static LONG WINAPI HandleException(LPEXCEPTION_POINTERS exceptionInfo)
{
	Crash::Handler handler = Crash::GetHandler();
	if (handler != 0) {
		handler();
	}

	if (Crash::IsMiniDumpEnabled()) {
		WriteDump(exceptionInfo);
	}

	if (previousExceptionFilter != 0) {
		return previousExceptionFilter(exceptionInfo);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

// static
void Crash::SetHandler(Crash::Handler handler) {
	Crash::handler_ = handler;
	if (handler != 0) {
		previousExceptionFilter = SetUnhandledExceptionFilter(HandleException);
	} else {
		SetUnhandledExceptionFilter(previousExceptionFilter);
	}
}

#else // _WIN32

#include <signal.h>

static void (*previousSignalHandler)(int);

static void HandleSIGSEGV(int sig)
{
	Crash::Handler handler = Crash::GetHandler();
	if (handler != 0) {
		handler();
	}
	signal(sig, SIG_DFL);
}

// static
void Crash::SetHandler(Crash::Handler handler) {
	Crash::handler_ = handler;
	if (handler != 0) {
		previousSignalHandler = signal(SIGSEGV, HandleSIGSEGV);
	} else {
		signal(SIGSEGV, previousSignalHandler);
	}
}

#endif // _WIN32

// static
Crash::Handler Crash::GetHandler() {
	return Crash::handler_;
}

// static
void Crash::EnableMiniDump(bool enable) {
	miniDumpEnabled_ = enable;
}

// static
bool Crash::IsMiniDumpEnabled() {
	return miniDumpEnabled_;
}
