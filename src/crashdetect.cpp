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
// // LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stack>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "amxcallstack.h"
#include "amxdebuginfo.h"
#include "amxpathfinder.h"
#include "amxutils.h"
#include "crashdetect.h"
#include "configreader.h"
#include "jump-x86.h"
#include "logprintf.h"
#include "os.h"
#include "plugincommon.h"
#include "version.h"
#include "x86callstack.h"

#include "amx/amx.h"
#include "amx/amxaux.h" // for amx_StrError()

#define AMX_EXEC_GDK (-10)

static inline std::string StripDirs(const std::string &filename) {
#if BOOST_VERSION >= 104600 || BOOST_FILESYSTEM_VERSION == 3
	return boost::filesystem::path(filename).filename().string();
#else
	return boost::filesystem::path(filename).filename();
#endif
}

bool crashdetect::errorCaught_ = false;
std::stack<crashdetect::NPCall> crashdetect::npCalls_;
ConfigReader crashdetect::serverCfg("server.cfg");
boost::unordered_map<AMX*, boost::shared_ptr<crashdetect> > crashdetect::instances_;

// static
bool crashdetect::Load(void **ppPluginData) {
	void *amxExecPtr = ((void**)ppPluginData[PLUGIN_DATA_AMX_EXPORTS])[PLUGIN_AMX_EXPORT_Exec];
	void *funAddr = JumpX86::GetTargetAddress(reinterpret_cast<unsigned char*>(amxExecPtr));
	if (funAddr == 0) {
		new JumpX86(amxExecPtr, (void*)AmxExec);
	} else {
		std::string module = StripDirs(os::GetModuleNameBySymbol(funAddr));
		if (!module.empty() && module != "samp-server.exe" && module != "samp03svr") {
			logprintf("  crashdetect must be loaded before %s", module.c_str());
			return false;
		}
	}

	os::SetCrashHandler(crashdetect::Crash);
	os::SetInterruptHandler(crashdetect::Interrupt);

	logprintf("  crashdetect v"CRASHDETECT_VERSION" is OK.");
	return true;
}

// static
int crashdetect::AmxLoad(AMX *amx) {
	if (instances_.find(amx) == instances_.end()) {
		instances_[amx].reset(new crashdetect(amx));
	}
	return AMX_ERR_NONE;
}

// static
int crashdetect::AmxUnload(AMX *amx) {
	instances_.erase(amx);
	return AMX_ERR_NONE;
}

// static
crashdetect *crashdetect::GetInstance(AMX *amx) {
	boost::unordered_map<AMX*, boost::shared_ptr<crashdetect> >::iterator 
			iterator = instances_.find(amx);
	if (iterator == instances_.end()) {
		crashdetect *inst = new crashdetect(amx);
		instances_.insert(std::make_pair(amx, boost::shared_ptr<crashdetect>(inst)));
		return inst;
	} 
	return iterator->second.get();
}

// static
int AMXAPI crashdetect::AmxDebug(AMX *amx) {
	return GetInstance(amx)->HandleAmxDebug();
}

// static
int AMXAPI crashdetect::AmxCallback(AMX *amx, cell index, cell *result, cell *params) {
	return GetInstance(amx)->HandleAmxCallback(index, result, params);
}

// static
int AMXAPI crashdetect::AmxExec(AMX *amx, cell *retval, int index) {
	return GetInstance(amx)->HandleAmxExec(retval, index);
}

// static
void crashdetect::Crash() {
	// Check if the last native/public call succeeded
	if (!npCalls_.empty()) {
		AMX *amx = npCalls_.top().amx();
		GetInstance(amx)->HandleCrash();
	} else {
		// Server/plugin internal error (in another thread?)
		logprintf("[debug] Server crashed due to an unknown error");
	}
}

// static
void crashdetect::RuntimeError(AMX *amx, cell index, int error) {
	GetInstance(amx)->HandleRuntimeError(index, error);
}

// static
void crashdetect::Interrupt() {	
	if (!npCalls_.empty()) {
		AMX *amx = npCalls_.top().amx();
		GetInstance(amx)->HandleInterrupt();
	} else {
		logprintf("[debug] Keyboard interrupt");
	}
	ExitOnError();
}

// static
void crashdetect::ExitOnError() {
	if (serverCfg.GetOption("die_on_error", false)) {
		logprintf("[debug] Aborting...");
		std::exit(EXIT_FAILURE);
	}
}

crashdetect::crashdetect(AMX *amx) 
	: amx_(amx)
	, amxhdr_(reinterpret_cast<AMX_HEADER*>(amx->base))
{
	AMXPathFinder pathFinder;
	pathFinder.AddSearchPath("gamemodes/");
	pathFinder.AddSearchPath("filterscripts/");

	boost::filesystem::path path;
	if (pathFinder.FindAMX(amx, path)) {
		amxPath_ = path.string();
#if BOOST_VERSION >= 104600 || BOOST_FILESYSTEM_VERSION == 3
		amxName_ = path.filename().string();
#else
		amxName_ = path.filename();
#endif
	}

	if (!amxPath_.empty()) {
		uint16_t flags;
		amx_Flags(amx_, &flags);
		if ((flags & AMX_FLAG_DEBUG) != 0) {
			debugInfo_.Load(amxPath_);
		}
	}

	amx_->sysreq_d = 0;

	prevDebugHook_ = amx_->debug;
	prevCallback_ = amx_->callback;

	amx_SetDebugHook(amx, AmxDebug);
	amx_SetCallback(amx, AmxCallback);
}

int crashdetect::HandleAmxDebug() {
	if (prevDebugHook_ != 0) {
		return prevDebugHook_(amx_);
	}
	return AMX_ERR_NONE;
}

int crashdetect::HandleAmxExec(cell *retval, int index) {
	npCalls_.push(NPCall(
		NPCall::PUBLIC, amx_, index, amx_->frm, amx_->cip));

	int retcode = ::amx_Exec(amx_, retval, index);
	if (retcode != AMX_ERR_NONE && !errorCaught_) {
		amx_Error(amx_, index, retcode);		
	} else {
		errorCaught_ = false;
	}

	npCalls_.pop();	
	return retcode;
}

int crashdetect::HandleAmxCallback(cell index, cell *result, cell *params) {
	npCalls_.push(NPCall(NPCall::NATIVE, amx_, index, amx_->frm, amx_->cip));
	int retcode = prevCallback_(amx_, index, result, params);	
	npCalls_.pop();
	return retcode;
}

void crashdetect::HandleRuntimeError(int index, int error) {
	crashdetect::errorCaught_ = true;
	if (error == AMX_ERR_INDEX && index == AMX_EXEC_GDK) {
		error = AMX_ERR_NONE;
	} else {
		logprintf("[debug] Run time error %d: \"%s\"", error, aux_StrError(error));
		switch (error) {
			case AMX_ERR_BOUNDS: {
				cell bound = *(reinterpret_cast<cell*>(amx_->cip + amx_->base + amxhdr_->cod + sizeof(cell)));
				cell index = amx_->pri;
				if (index < 0) {
					logprintf("[debug]   Accessing element at negative index %d", index);
				} else {
					logprintf("[debug]   Accessing element at index %d past array upper bound %d", index, bound);
				}
				break;
			}
			case AMX_ERR_NOTFOUND: {
				logprintf("[debug]   The following natives are not registered:");
				AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(amx_->base + amxhdr_->natives);
				int numNatives = 0;
				amx_NumNatives(amx_, &numNatives);
				for (int i = 0; i < numNatives; ++i) {
					if (natives[i].address == 0) {
						char *name = reinterpret_cast<char*>(natives[i].nameofs + amx_->base);
						logprintf("[debug]     %s", name);
					}
				}
				break;
			}
			case AMX_ERR_STACKERR:
				logprintf("[debug]   Stack index (STK) is 0x%X, heap index (HEA) is 0x%X", amx_->stk, amx_->hea); 
				break;
			case AMX_ERR_STACKLOW:
				logprintf("[debug]   Stack index (STK) is 0x%X, stack top (STP) is 0x%X", amx_->stk, amx_->stp);
				break;
			case AMX_ERR_HEAPLOW:
				logprintf("[debug]   Heap index (HEA) is 0x%X, heap bottom (HLW) is 0x%X", amx_->hea, amx_->hlw);
				break;
			case AMX_ERR_INVINSTR: {
				cell opcode = *(reinterpret_cast<cell*>(amx_->cip + amx_->base + amxhdr_->cod));
				logprintf("[debug]   Unknown opcode 0x%x at address 0x%08X", opcode , amx_->cip);
				break;
			}
		}
		if (error != AMX_ERR_NOTFOUND &&
		    error != AMX_ERR_INDEX &&
		    error != AMX_ERR_CALLBACK &&
		    error != AMX_ERR_INIT) 
		{
			PrintBacktrace();
		}
		ExitOnError();
	}
}

void crashdetect::HandleCrash() {
	logprintf("[debug] Server crashed while executing %s", amxName_.c_str());
	PrintBacktrace();
	PrintThreadBacktrace();
}

void crashdetect::HandleInterrupt() {
	logprintf("[debug] Keyboard interrupt");
	PrintBacktrace();
}

void crashdetect::PrintBacktrace() const {
	if (npCalls_.empty()) {
		return;
	}

	logprintf("[debug] Backtrace:");

	std::stack<NPCall> npCallStack = npCalls_;	
	cell frm = amx_->frm;
	cell cip = amx_->cip;
	int level = 0;

	while (!npCallStack.empty()) {
		NPCall call = npCallStack.top();

		if (call.amx() != amx_) {
			assert(level != 0);
			break;
		}

		if (call.type() == NPCall::NATIVE) {			
			AMX_NATIVE address = amxutils::GetNativeAddress(call.amx(), call.index());
			if (address != 0) {				
				std::string module = StripDirs(os::GetModuleNameBySymbol((void*)address));
				std::string from = " from " + module;
				if (module.empty()) {
					from.clear();
				}				
				const char *name = amxutils::GetNativeName(call.amx(), call.index());
				if (name != 0) {
					logprintf("[debug] #%-2d native %s ()%s", level++, name, from.c_str());
				}
			}
		} else if (call.type() == NPCall::PUBLIC) {
			AMXDebugInfo &debugInfo = instances_[call.amx()]->debugInfo_;

			amxutils::PushStack(call.amx(), cip); // push return address
			amxutils::PushStack(call.amx(), frm); // push frame pointer
			frm = call.amx()->stk;

			AMXCallStack callStack(call.amx(), debugInfo, frm);
			std::deque<AMXStackFrame> frames = callStack.GetFrames();

			frm = amxutils::PopStack(call.amx()); // pop frame pointer
			cip = amxutils::PopStack(call.amx()); // pop return address

			if (!debugInfo.IsLoaded()) {
				// Edit address of entry point.
				AMXStackFrame &bottom = frames.back();
				bottom = AMXStackFrame(call.amx(), 
					bottom.GetFrameAddress(), 
					bottom.GetReturnAddress(),
					amxutils::GetPublicAddress(call.amx(), call.index()),
					debugInfo);
			}

			std::string &amxName = instances_[call.amx()]->amxName_;
			for (size_t i = 0; i < frames.size(); i++) {
				std::string from = " from " + amxName;
				if (amxName.empty() || debugInfo.IsLoaded()) {
					from.clear();
				}
				logprintf("[debug] #%-2d %s%s", level++, frames[i].GetString().c_str(), from.c_str());
			}

			frm = call.frm();
			cip = call.cip();
		}
		npCallStack.pop();		
	}
}

void crashdetect::PrintThreadBacktrace() const {
	logprintf("[debug] Thread backtrace:");

	int level = 0;

	std::deque<X86StackFrame> frames = X86CallStack().GetFrames();	
	for (std::deque<X86StackFrame>::const_iterator iterator = frames.begin();
			iterator != frames.end(); ++iterator) {
		const X86StackFrame &frame = *iterator;

		std::string module = os::GetModuleNameBySymbol(frame.GetReturnAddress());
		std::string from = " from " + module;
		if (module.empty()) {
			from.clear();
		}

		logprintf("[debug] #%-2d %08x in ?? ()%s", level++, frame.GetReturnAddress(), from.c_str());
	}
}
