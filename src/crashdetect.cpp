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
#include <memory>
#include <stack>
#include <string>
#include <vector>
#include <unordered_map>

#include "amxdebuginfo.h"
#include "amxpathfinder.h"
#include "amxstacktrace.h"
#include "amxutils.h"
#include "configreader.h"
#include "crashdetect.h"
#include "fileutils.h"
#include "logprintf.h"
#include "os.h"
#include "plugincommon.h"
#include "x86stacktrace.h"

#include "amx/amx.h"
#include "amx/amxaux.h" // for amx_StrError()

#define AMX_EXEC_GDK (-10)

bool crashdetect::errorCaught_ = false;
std::stack<crashdetect::NPCall> crashdetect::npCalls_;
ConfigReader crashdetect::serverCfg("server.cfg");
crashdetect::InstanceMap crashdetect::instances_;

// static
std::weak_ptr<crashdetect> crashdetect::GetInstance(AMX *amx) {
	InstanceMap::const_iterator iterator = instances_.find(amx);
	if (iterator == instances_.end()) {
		std::shared_ptr<crashdetect> instance(new crashdetect(amx));
		instances_.insert(std::make_pair(amx, instance));
		return instance;
	} 
	return iterator->second;
}

// static
void crashdetect::DestroyInstance(AMX *amx) {
	instances_.erase(amx);
}

// static
void crashdetect::Crash() {
	// Check if the last native/public call succeeded
	if (!npCalls_.empty()) {
		AMX *amx = npCalls_.top().amx();
		GetInstance(amx).lock()->HandleCrash();
	} else {
		// Server/plugin internal error (in another thread?)
		logprintf("[debug] Server crashed due to an unknown error");
	}
	PrintThreadBacktrace(4);
}

// static
void crashdetect::RuntimeError(AMX *amx, cell index, int error) {
	GetInstance(amx).lock()->HandleRuntimeError(index, error);
}

// static
void crashdetect::Interrupt() {	
	if (!npCalls_.empty()) {
		AMX *amx = npCalls_.top().amx();
		GetInstance(amx).lock()->HandleInterrupt();
	} else {
		logprintf("[debug] Server recieved interrupt signal");
	}	
	PrintThreadBacktrace(3);
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

	amxPath_ = pathFinder.FindAMX(amx);
	amxName_ = fileutils::GetFileName(amxPath_);

	if (!amxPath_.empty()) {
		uint16_t flags;
		amx_Flags(amx_, &flags);
		if ((flags & AMX_FLAG_DEBUG) != 0) {
			debugInfo_.Load(amxPath_);
		}
	}

	amx_->sysreq_d = 0;
	prevCallback_ = amx_->callback;	
}

int crashdetect::HandleAmxExec(cell *retval, int index) {
	npCalls_.push(NPCall(NPCall::PUBLIC, amx_, index, amx_->frm, amx_->cip));

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
			PrintAmxBacktrace();			
		}
		ExitOnError();
	}
}

void crashdetect::HandleCrash() {
	logprintf("[debug] Server crashed while executing %s", amxName_.c_str());
	PrintAmxBacktrace();
}

void crashdetect::HandleInterrupt() {
	logprintf("[debug]: Server recieved interrupt signal while executing %s", amxName_.c_str());
	PrintAmxBacktrace();
}

// static
void crashdetect::PrintAmxBacktrace() {
	if (npCalls_.empty()) {
		return;
	}

	logprintf("[debug] Backtrace:");

	std::stack<NPCall> npCallStack = npCalls_;
	cell frm = npCallStack.top().amx()->frm;
	cell cip = npCallStack.top().amx()->cip;
	int level = 0;

	while (!npCallStack.empty()) {
		NPCall call = npCallStack.top();

		if (call.amx() != npCallStack.top().amx()) {
			assert(level != 0);
			break;
		}

		if (call.type() == NPCall::NATIVE) {			
			AMX_NATIVE address = amxutils::GetNativeAddress(call.amx(), call.index());
			if (address != 0) {				
				std::string module = fileutils::GetFileName(os::GetModulePath((void*)address));
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

			AMXStackTrace trace(call.amx(), debugInfo, frm);
			std::deque<AMXStackFrame> frames = trace.GetFrames();

			frm = amxutils::PopStack(call.amx()); // pop frame pointer
			cip = amxutils::PopStack(call.amx()); // pop return address

			if (frames.empty()) {
				ucell epAddr = amxutils::GetPublicAddress(call.amx(), call.index());
				frames.push_front(AMXStackFrame(call.amx(), frm, cip, epAddr, debugInfo));
			} else {
				if (!debugInfo.IsLoaded()) {
					AMXStackFrame &bottom = frames.back();
					bottom = AMXStackFrame(call.amx(), 
						bottom.GetFrameAddress(), 
						bottom.GetReturnAddress(),
						amxutils::GetPublicAddress(call.amx(), call.index()),
						debugInfo);
				}
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

// static
void crashdetect::PrintThreadBacktrace(int framesToSkip) {
	logprintf("[debug] Thread backtrace:");

	int level = 0;

	std::deque<X86StackFrame> frames = X86StackTrace(framesToSkip).GetFrames();	
	for (std::deque<X86StackFrame>::const_iterator iterator = frames.begin();
			iterator != frames.end(); ++iterator) {
		const X86StackFrame &frame = *iterator;

		std::string module = os::GetModulePath(frame.GetReturnAddress());
		std::string from = " from " + module;
		if (module.empty()) {
			from.clear();
		}

		logprintf("[debug] #%-2d %s%s", level++, frame.GetString().c_str(), from.c_str());
	}
}
