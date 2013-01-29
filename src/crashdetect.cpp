// Copyright (c) 2011-2013 Zeex
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

#include <amx/amx.h>
#include <amx/amxaux.h>

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

static const int kOpBounds  = 121;
static const int kOpSysreqC = 123;

bool CrashDetect::error_detected_ = false;
std::stack<NPCall*> CrashDetect::np_calls_;;

// static
void CrashDetect::OnException(void *context) {
	if (!np_calls_.empty()) {
		CrashDetect::Get(np_calls_.top()->amx())->HandleException();
	} else {
		Printf("Server crashed due to an unknown error");
	}
	PrintSystemBacktrace(context);
}

// static
void CrashDetect::OnInterrupt(void *context) {
	if (!np_calls_.empty()) {
		CrashDetect::Get(np_calls_.top()->amx())->HandleInterrupt();
	} else {
		Printf("Server received interrupt signal");
	}
	PrintSystemBacktrace(context);
}

// static
void CrashDetect::DieOrContinue() {
	if (server_cfg_.GetOption("die_on_error", false)) {
		Printf("Aborting...");
		std::exit(EXIT_FAILURE);
	}
}

CrashDetect::CrashDetect(AMX *amx)
	: AMXService<CrashDetect>(amx)
	, amx_(amx)
	, prev_callback_(0)
	, server_cfg_("server.cfg")
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

	amx_path_ = pathFinder.FindAMX(this->amx());
	amx_name_ = fileutils::GetFileName(amx_path_);

	if (!amx_path_.empty() && AMXDebugInfo::IsPresent(this->amx())) {
		debug_info_.Load(amx_path_);
	}

	amx_.DisableSysreqD();
	prev_callback_ = amx_.GetCallback();

	return AMX_ERR_NONE;
}

int CrashDetect::Unload() {
	return AMX_ERR_NONE;
}

int CrashDetect::DoAmxCallback(cell index, cell *result, cell *params) {
	NPCall call = NPCall::Native(amx(), index);
	np_calls_.push(&call);
	int error = prev_callback_(amx(), index, result, params);
	np_calls_.pop();
	return error;
}

int CrashDetect::DoAmxExec(cell *retval, int index) {	
	NPCall call = NPCall::Public(amx(), index);
	np_calls_.push(&call);

	int error = ::amx_Exec(amx(), retval, index);
	if (error != AMX_ERR_NONE && !error_detected_) {
		HandleExecError(index, error);
	} else {
		error_detected_ = false;
	}

	np_calls_.pop();
	return error;
}

void CrashDetect::HandleExecError(int index, const AMXError &error) {
	CrashDetect::error_detected_ = true;

	if (error.code() == AMX_ERR_INDEX && index == AMX_EXEC_GDK) {
		return;
	}

	Printf("Run time error %d: \"%s\"", error, error.GetString());

	switch (error.code()) {
		case AMX_ERR_BOUNDS: {
			cell *ip = reinterpret_cast<cell*>(amx_.GetCode() + amx_.GetCip());
			cell opcode = *ip;
			if (opcode == kOpBounds) {
				cell bound = *(ip + 1);
				cell index = amx_.GetPri();
				if (index < 0) {
					Printf(" Accessing element at negative index %d", index);
				} else {
					Printf(" Accessing element at index %d past array upper bound %d", index, bound);
				}
			}
			break;
		}
		case AMX_ERR_NOTFOUND: {
			AMX_FUNCSTUBNT *natives = amx_.GetNatives();
			int num_natives = amx_.GetNumNatives();
			for (int i = 0; i < num_natives; ++i) {
				if (natives[i].address == 0) {
					Printf(" %s", amx_.GetName(natives[i].nameofs));
				}
			}
			break;
		}
		case AMX_ERR_STACKERR:
			Printf(" Stack pointer (STK) is 0x%X, heap pointer (HEA) is 0x%X", amx_.GetStk(), amx_.GetHea());
			break;
		case AMX_ERR_STACKLOW:
			Printf(" Stack pointer (STK) is 0x%X, stack top (STP) is 0x%X", amx_.GetStk(), amx_.GetStp());
			break;
		case AMX_ERR_HEAPLOW:
			Printf(" Heap pointer (HEA) is 0x%X, heap bottom (HLW) is 0x%X", amx_.GetHea(), amx_.GetHlw());
			break;
		case AMX_ERR_INVINSTR: {
			cell opcode = *(reinterpret_cast<cell*>(amx_.GetCode() + amx_.GetCip()));
			Printf(" Unknown opcode 0x%x at address 0x%08X", opcode , amx_.GetCip());
			break;
		}
		case AMX_ERR_NATIVE: {
			cell *ip = reinterpret_cast<cell*>(amx_.GetCode() + amx_.GetCip());
			cell opcode = *(ip - 2);
			if (opcode == kOpSysreqC) {
				cell index = *(ip - 1);
				Printf(" %s", amx_.GetNativeName(index));
			}
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

	std::string command = server_cfg_.GetOption("run_on_error", std::string());
	if (!command.empty()) {
		std::system(command.c_str());
	}

	DieOrContinue();
}

void CrashDetect::HandleException() {
	Printf("Server crashed while executing %s", amx_name_.c_str());
	PrintAmxBacktrace();
}

void CrashDetect::HandleInterrupt() {
	Printf("Server received interrupt signal while executing %s", amx_name_.c_str());
	PrintAmxBacktrace();
}

// static
void CrashDetect::PrintAmxBacktrace() {
	if (np_calls_.empty()) {
		return;
	}

	AMXScript topAmx = np_calls_.top()->amx();

	cell frm = topAmx.GetFrm();
	cell cip = topAmx.GetCip();
	int level = 0;

	if (cip == 0) {
		return;
	}

	Printf("AMX backtrace:");

	std::stack<NPCall*> np_calls = np_calls_;

	while (!np_calls.empty() && cip != 0) {
		const NPCall *call = np_calls.top();
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
					Printf("#%d native %s () [%08x]%s", level++, name, address, from.c_str());
				}
			}
		} else if (call->IsPublic()) {
			AMXDebugInfo &debug_info = CrashDetect::Get(amx)->debug_info_;

			amx.PushStack(cip); // push return address
			amx.PushStack(frm); // push frame pointer

			std::list<AMXStackFrame> frames;
			{
				AMXStackTrace trace(amx, amx.GetStk(), &debug_info);
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
				ucell ep_addr = amx.GetPublicAddr(call->index());
				frames.push_front(AMXStackFrame(amx, frm, 0, ep_addr, &debug_info));
			} else {
				if (!debug_info.IsLoaded()) {
					AMXStackFrame &bottom = frames.back();
					bottom = AMXStackFrame(amx,
						bottom.GetFrameAddr(),
						bottom.GetRetAddr(),
						amx.GetPublicAddr(call->index()),
						&debug_info);
				}
			}

			std::string &amx_name = CrashDetect::Get(amx)->amx_name_;
			for (std::list<AMXStackFrame>::const_iterator iterator = frames.begin();
					iterator != frames.end(); ++iterator)
			{
				std::string from = " from " + amx_name;
				if (amx_name.empty() || debug_info.IsLoaded()) {
					from.clear();
				}
				Printf("#%d %s%s", level++, iterator->AsString().c_str(), from.c_str());
			}

			frm = call->frm();
			cip = call->cip();
		}

		np_calls.pop();
	}
}

// static
void CrashDetect::PrintSystemBacktrace(void *context) {
	Printf("System backtrace:");

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

		Printf("#%d %s%s", level++, frame.AsString().c_str(), from.c_str());
	}
}

// static
void CrashDetect::Printf(const char *format, ...) {
	std::va_list va;
	va_start(va, format);

	std::string new_format;
	new_format.append("[debug] ");
	new_format.append(format);

	vlogprintf(new_format.c_str(), va);
	va_end(va);
}
