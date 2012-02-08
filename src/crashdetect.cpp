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

#include "amx/amx.h"
#include "amx/amxaux.h" // for amx_StrError()

static inline std::string StripDirs(const std::string &filename) {
	return boost::filesystem::path(filename).filename().string();
}

bool crashdetect::errorCaught_ = false;
std::stack<crashdetect::NativePublicCall> crashdetect::npCalls_;
ConfigReader crashdetect::serverCfg("server.cfg");
boost::unordered_map<AMX*, boost::shared_ptr<crashdetect> > crashdetect::instances_;

// static
bool crashdetect::Load(void **ppPluginData) {
	// Hook amx_Exec to catch execution errors
	void *amxExecPtr = ((void**)ppPluginData[PLUGIN_DATA_AMX_EXPORTS])[PLUGIN_AMX_EXPORT_Exec];

	// But first make sure it's not already hooked by someone else
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

	// Set crash handler
	os::SetCrashHandler(crashdetect::Crash);

	// Set Ctrl-C signal handler
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
	// Try to determine .amx file name.
	AMXPathFinder pathFinder;
	pathFinder.AddSearchPath("gamemodes/");
	pathFinder.AddSearchPath("filterscripts/");

	boost::filesystem::path path;
	if (pathFinder.FindAMX(amx, path)) {		
		amxPath_ = path.string();
		amxName_ = path.filename().string();
	}

	if (!amxPath_.empty()) {
		uint16_t flags;
		amx_Flags(amx_, &flags);
		if ((flags & AMX_FLAG_DEBUG) != 0) {
			debugInfo_.Load(amxPath_);
		}
	}

	// Disable overriding of SYSREQ.C opcodes by SYSREQ.D
	amx_->sysreq_d = 0;

	// Remember previously set callback and debug hook
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
	npCalls_.push(NativePublicCall(
		NativePublicCall::PUBLIC, amx_, index, amx_->frm));

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
	npCalls_.push(NativePublicCall(
		NativePublicCall::NATIVE, amx_, index, amx_->frm));

	// Reset error
	amx_->error = AMX_ERR_NONE;

	// Call any previously set callback (amx_Callback by default)
	int retcode = prevCallback_(amx_, index, result, params);	

	// Check if the AMX_ERR_NATIVE error is set
	if (amx_->error == AMX_ERR_NATIVE) {
		HandleNativeError(index);
	}

	// Reset error again
	amx_->error = AMX_ERR_NONE;

	npCalls_.pop();
	return retcode;
}

void crashdetect::HandleNativeError(int index) {
	const char *name = amxutils::GetNativeName(amx_, index);
	if (name == 0) {
		name = "<unknown native>";
	}		
	if (debugInfo_.IsLoaded()) {
		logprintf("[debug] Native function %s() failed (AMX_ERR_NATIVE is set)", name);
	} else {
		logprintf("[debug] Native function %s() failed (AMX_ERR_NATIVE is set)", name);
	}
	PrintBacktrace();
	ExitOnError();
}

void crashdetect::HandleRuntimeError(int index, int error) {
	crashdetect::errorCaught_ = true;
	if (error == AMX_ERR_INDEX && index == AMX_EXEC_GDK) {
		// Fail silently as this public doesn't really exist
		error = AMX_ERR_NONE;
	} else {
		logprintf("[debug] Run time error %d: \"%s\"", error, aux_StrError(error));
		switch (error) {
			case AMX_ERR_BOUNDS: {
				cell bound = *(reinterpret_cast<cell*>(amx_->cip + amx_->base + amxhdr_->cod));
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
				logprintf("[debug]   Invalid opcode 0x%X at address 0x%X", opcode , amx_->cip - sizeof(cell));
				break;
			}
		}
		// Do not print backtrace on certain errors as it has no effect.
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
}

void crashdetect::HandleInterrupt() {
	logprintf("[debug] Keyboard interrupt");
	PrintBacktrace();
}

void crashdetect::PrintBacktrace() const {
	if (npCalls_.empty()) {
		return;
	}

	logprintf("[debug] Backtrace (most recent call first):");

	std::stack<NativePublicCall> npCallStack = npCalls_;
	int depth = 0;

	while (!npCallStack.empty()) {
		NativePublicCall call = npCallStack.top();		

		if (call.type() == NativePublicCall::NATIVE) {			
			AMX_NATIVE address = amxutils::GetNativeAddress(call.amx(), call.index());
			if (address != 0) {				
				std::string module = StripDirs(os::GetModuleNameBySymbol((void*)address));
				if (module.empty()) {
					module.assign("??");
				}
				const char *name = amxutils::GetNativeName(call.amx(), call.index());
				if (name != 0) {
					logprintf("[debug] #%-2d native %s() from %s", depth, name, module.c_str());
				} else {
					logprintf("[debug] #%-2d native %08x() from %s", depth, address, module.c_str());
				}				
			}
			++depth;
		} 
		else if (call.type() == NativePublicCall::PUBLIC) {
			AMXDebugInfo &debugInfo = instances_[call.amx()]->debugInfo_;

			std::string &amxName = instances_[call.amx()]->amxName_;
			if (amxName.empty()) {
				amxName.assign("??");
			}

			std::vector<AMXStackFrame> frames = AMXCallStack(call.amx(), debugInfo, call.amx()->frm).GetFrames();
			if (frames.empty()) {
				AMXStackFrame fakeFrame(call.amx(), 0, 0,
					amxutils::GetPublicAddress(call.amx(), call.index()), debugInfo);
				frames.push_back(fakeFrame);
			}

			if (debugInfo.IsLoaded()) {
				for (size_t i = 0; i < frames.size(); i++) {								
					AMXStackFrame &frame = frames[i];					
					if (i > 0) {
						AMXStackFrame &prevFrame = frames[i - 1];
						logprintf("[debug] #%-2d %s+0x%x at %s:%ld ", depth,
								frame.GetFunctionPrototype().c_str(),
								prevFrame.GetCallAddress() - frame.GetFunctionAddress(),
								StripDirs(frame.GetSourceFileName()).c_str(), 
								debugInfo.GetLineNumber(prevFrame.GetCallAddress()));
					} else {
						logprintf("[debug] #%-2d %s+0x%x at %s:%ld", depth,
								frame.GetFunctionPrototype().c_str(),
								call.amx()->cip - frame.GetFunctionAddress(),
								StripDirs(debugInfo.GetFileName(call.amx()->cip)).c_str(),
								debugInfo.GetLineNumber(call.amx()->cip));
					}
					++depth;
				}
			} else { 
				// Have no debug info. This means we don't know line numbers/source files, function names
				// work only for publics, etc.
				for (size_t i = 0; i < frames.size(); i++) {								
					AMXStackFrame frame = frames[i];
					ucell offset = 0;
					if (i > 0) {
						AMXStackFrame &prevFrame = frames[i - 1];
						offset = prevFrame.GetCallAddress() - frame.GetFunctionAddress();
						if (frame.GetFunctionAddress() == 0) {
							if (i == frames.size() - 1) {
								// Reached the top frame (entry point).
								frame = AMXStackFrame(call.amx(), 0, 0, 
										amxutils::GetPublicAddress(call.amx(), call.index()));
							} else {
								logprintf("[debug] Stack corrupted");
								break;
							}
						}
					} else {
						offset = call.amx()->cip - frame.GetFunctionAddress();
						if (frames.size() == 1) {
							// This is the first and the only frame.
							frame = AMXStackFrame(call.amx(), 0, 0, 
									amxutils::GetPublicAddress(call.amx(), call.index()));						
						}
					}
					logprintf("[debug] #%-2d %s+0x%x from %s", depth,
							frame.GetFunctionPrototype().c_str(), offset, amxName.c_str());
					++depth;
				}				
			}
		}
		npCallStack.pop();		
	}
}

