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

#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <list>
#include <map>
#include <stack>
#include <string>

#include "amx.h"
#include "amxdebuginfo.h"
#include "amxerror.h"
#include "amxpathfinder.h"
#include "amxscript.h"
#include "amxstacktrace.h"
#include "compiler.h"
#include "configreader.h"
#include "crashdetect.h"
#include "fileutils.h"
#include "logprintf.h"
#include "npcall.h"
#include "os.h"
#include "stacktrace.h"

#define AMX_EXEC_GDK (-10)

bool CrashDetect::errorCaught_ = false;
std::stack<NPCall*> CrashDetect::npCalls_;;

// static
void CrashDetect::OnException(void *context) {
	if (!npCalls_.empty()) {
		CrashDetect::Get(npCalls_.top()->amx())->HandleException();
	} else {
		logprintf("Server crashed due to an unknown error");
	}
	PrintSystemBacktrace(context);
}

// static
void CrashDetect::OnInterrupt(void *context) {
	if (!npCalls_.empty()) {
		CrashDetect::Get(npCalls_.top()->amx())->HandleInterrupt();
	} else {
		logprintf("Server received interrupt signal");
	}
	PrintSystemBacktrace(context);
}

// static
void CrashDetect::DieOrContinue() {
	if (serverCfg.GetOption("die_on_error", false)) {
		logprintf("Aborting...");
		std::exit(EXIT_FAILURE);
	}
}

CrashDetect::CrashDetect(AMX *amx)
	: AMXService<CrashDetect>(amx)
	, amx_(amx)
	, prevCallback_(0)
	, serverCfg("server.cfg")
{
}

int CrashDetect::Load() {
	AMXPathFinder pathFinder;
	pathFinder.AddSearchPath("gamemodes");
	pathFinder.AddSearchPath("filterscripts");

	// Read a list of additional search paths from AMX_PATH.
	const char *AMX_PATH = getenv("AMX_PATH");
	if (AMX_PATH != 0) {
		std::string var(AMX_PATH);
		std::string path;
		std::string::size_type begin = 0;
		while (begin < var.length()) {
			std::string::size_type end = var.find(fileutils::kNativePathListSepChar, begin);
			if (end == std::string::npos) {
				end = var.length();
			}
			path.assign(var.begin() + begin, var.begin() + end);
			if (!path.empty()) {
				pathFinder.AddSearchPath(path);
			}
			begin = end + 1;
		}
	}

	amxPath_ = pathFinder.FindAMX(this->amx());
	amxName_ = fileutils::GetFileName(amxPath_);

	if (!amxPath_.empty() && AMXDebugInfo::IsPresent(this->amx())) {
		debugInfo_.Load(amxPath_);
	}

	amx_.DisableSysreqD();
	prevCallback_ = amx_.GetCallback();

	return AMX_ERR_NONE;
}

int CrashDetect::Unload() {
	return AMX_ERR_NONE;
}

int CrashDetect::DoAmxCallback(cell index, cell *result, cell *params) {
	NPCall call = NPCall::Native(amx(), index);
	npCalls_.push(&call);
	int retcode = prevCallback_(amx(), index, result, params);
	npCalls_.pop();
	return retcode;
}

int CrashDetect::DoAmxExec(cell *retval, int index) {	
	NPCall call = NPCall::Public(amx(), index);
	npCalls_.push(&call);

	int retcode = ::amx_Exec(amx(), retval, index);
	if (retcode != AMX_ERR_NONE && !errorCaught_) {
		amx_Error(amx(), index, retcode);
	} else {
		errorCaught_ = false;
	}

	npCalls_.pop();
	return retcode;
}

int CrashDetect::DoAmxRelease(cell amx_addr, void *releaser) {
	if (amx_addr < amx_.GetHlw() || amx_addr >= amx_.GetStk()) {
		HandleReleaseError(amx_addr, releaser);
	}
	return amx_Release(amx(), amx_addr);
}

void CrashDetect::HandleExecError(int index, const AMXError &error) {
	CrashDetect::errorCaught_ = true;

	if (error.code() == AMX_ERR_INDEX && index == AMX_EXEC_GDK) {
		return;
	}

	logprintf("Run time error %d: \"%s\"", error, error.string());

	switch (error.code()) {
		case AMX_ERR_BOUNDS: {
			cell bound = *(reinterpret_cast<cell*>(amx_.GetCode() + amx_.GetCip() + sizeof(cell)));
			cell index = amx_.GetPri();
			if (index < 0) {
				logprintf(" Accessing element at negative index %d", index);
			} else {
				logprintf(" Accessing element at index %d past array upper bound %d", index, bound);
			}
			break;
		}
		case AMX_ERR_NOTFOUND: {
			AMX_FUNCSTUBNT *natives = amx_.GetNatives();
			int numNatives = amx_.GetNumNatives();
			for (int i = 0; i < numNatives; ++i) {
				if (natives[i].address == 0) {
					logprintf(" %s", amx_.GetName(natives[i].nameofs));
				}
			}
			break;
		}
		case AMX_ERR_STACKERR:
			logprintf(" Stack pointer (STK) is 0x%X, heap pointer (HEA) is 0x%X", amx_.GetStk(), amx_.GetHea());
			break;
		case AMX_ERR_STACKLOW:
			logprintf(" Stack pointer (STK) is 0x%X, stack top (STP) is 0x%X", amx_.GetStk(), amx_.GetStp());
			break;
		case AMX_ERR_HEAPLOW:
			logprintf(" Heap pointer (HEA) is 0x%X, heap bottom (HLW) is 0x%X", amx_.GetHea(), amx_.GetHlw());
			break;
		case AMX_ERR_INVINSTR: {
			cell opcode = *(reinterpret_cast<cell*>(amx_.GetCode() + amx_.GetCip()));
			logprintf(" Unknown opcode 0x%x at address 0x%08X", opcode , amx_.GetCip());
			break;
		}
	}

	if (error.code() != AMX_ERR_NOTFOUND &&
		error.code() != AMX_ERR_INDEX &&
		error.code() != AMX_ERR_CALLBACK &&
		error.code() != AMX_ERR_INIT)
	{
		PrintAmxBacktrace();
	}

	std::string command = serverCfg.GetOption("run_on_error", std::string());
	if (!command.empty()) {
		std::system(command.c_str());
	}

	DieOrContinue();
}

void CrashDetect::HandleException() {
	logprintf("Server crashed while executing %s", amxName_.c_str());
	PrintAmxBacktrace();
}

void CrashDetect::HandleInterrupt() {
	logprintf("Server received interrupt signal while executing %s", amxName_.c_str());
	PrintAmxBacktrace();
}

void CrashDetect::HandleReleaseError(cell address, void *releaser) {
	std::string plugin = fileutils::GetFileName(os::GetModulePathFromAddr(releaser));
	if (plugin.empty()) {
		plugin.assign("<unknown>");
	}
	logprintf("Bad heap release detected:");
	logprintf(" %s [%08x] is releasing memory at %08x which is out of heap", plugin.c_str(), releaser, address);
	PrintSystemBacktrace();
}

// static
void CrashDetect::PrintAmxBacktrace() {
	if (npCalls_.empty()) {
		return;
	}

	AMXScript topAmx = npCalls_.top()->amx();

	cell frm = topAmx.GetFrm();
	cell cip = topAmx.GetCip();
	int level = 0;

	if (cip == 0) {
		return;
	}

	logprintf("AMX backtrace:");

	std::stack<NPCall*> npCalls = npCalls_;

	while (!npCalls.empty() && cip != 0) {
		const NPCall *call = npCalls.top();
		AMXScript amx = call->amx();

		// We don't trace calls across AMX bounds i.e. outside of top-level
		// function's AMX instance.
		if (amx != topAmx) {
			assert(level != 0);
			break;
		}

		if (call->IsNative()) {
			AMX_NATIVE address = reinterpret_cast<AMX_NATIVE>(amx.GetNativeAddr(call->index()));
			if (address != 0) {
				std::string module = fileutils::GetFileName(os::GetModulePathFromAddr((void*)address));
				std::string from = " from " + module;
				if (module.empty()) {
					from.clear();
				}
				const char *name = amx.GetNativeName(call->index());
				if (name != 0) {
					logprintf("#%d native %s () [%08x]%s", level++, name, address, from.c_str());
				}
			}
		} else if (call->IsPublic()) {
			AMXDebugInfo &debugInfo = CrashDetect::Get(amx)->debugInfo_;

			amx.PushStack(cip); // push return address
			amx.PushStack(frm); // push frame pointer

			std::list<AMXStackFrame> frames;
			{
				AMXStackTrace trace(amx, amx.GetStk(), &debugInfo);
				do {
					AMXStackFrame frame = trace.GetFrame();
					if (frame) {
						frames.push_back(frame);
					} else {
						break;
					}
				} while (trace.Next());
			}

			frm = amx.PopStack(); // pop frame pointer
			cip = amx.PopStack(); // pop return address

			if (frames.empty()) {
				ucell epAddr = amx.GetPublicAddr(call->index());
				frames.push_front(AMXStackFrame(amx, frm, 0, epAddr, &debugInfo));
			} else {
				if (!debugInfo.IsLoaded()) {
					AMXStackFrame &bottom = frames.back();
					bottom = AMXStackFrame(amx,
						bottom.GetFrameAddr(),
						bottom.GetRetAddr(),
						amx.GetPublicAddr(call->index()),
						&debugInfo);
				}
			}

			std::string &amxName = CrashDetect::Get(amx)->amxName_;
			for (std::list<AMXStackFrame>::const_iterator iterator = frames.begin();
					iterator != frames.end(); ++iterator)
			{
				std::string from = " from " + amxName;
				if (amxName.empty() || debugInfo.IsLoaded()) {
					from.clear();
				}
				logprintf("#%d %s%s", level++, iterator->AsString().c_str(), from.c_str());
			}

			frm = call->frm();
			cip = call->cip();
		}

		npCalls.pop();
	}
}

// static
void CrashDetect::PrintSystemBacktrace(void *context) {
	logprintf("System backtrace:");

	int level = 0;

	StackTrace trace(context);
	std::deque<StackFrame> frames = trace.GetFrames();

	for (std::deque<StackFrame>::const_iterator iterator = frames.begin();
			iterator != frames.end(); ++iterator) {
		const StackFrame &frame = *iterator;

		std::string module = os::GetModulePathFromAddr(frame.GetRetAddr());
		std::string from = " from " + module;
		if (module.empty()) {
			from.clear();
		}

		logprintf("#%d %s%s", level++, frame.AsString().c_str(), from.c_str());
	}
}

// static
void CrashDetect::logprintf(const char *format, ...) {
	std::va_list va;
	va_start(va, format);

	std::string newFormat;
	newFormat.append("[debug] ");
	newFormat.append(format);

	vlogprintf(newFormat.c_str(), va);
	va_end(va);
}
