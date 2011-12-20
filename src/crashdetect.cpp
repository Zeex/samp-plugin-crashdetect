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
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#else
	#ifndef _GNU_SOURCE
		#define _GNU_SOURCE 1 // for dladdr()
	#endif
	#include <dlfcn.h> 
#endif

#include "amxcallstack.h"
#include "amxdebuginfo.h"
#include "amxname.h"
#include "crash.h"
#include "crashdetect.h"
#include "jump-x86.h"
#include "plugincommon.h"
#include "version.h"

#include "amx/amx.h"
#include "amx/amxaux.h" // for amx_StrError()

typedef void (*logprintf_t)(const char *fmt, ...);	
static logprintf_t logprintf;

static boost::unordered_map<AMX*, boost::shared_ptr<Crashdetect> > instances;

// Gets called before amx_Exec() on error but before AMX stack/heap is reset
extern "C" int AMXAPI amx_Error(AMX *amx, cell index, int error) {
	if (error != AMX_ERR_NONE) {
		instances[amx]->HandleRuntimeError(index, error);
	}
	return AMX_ERR_NONE;
}

static void *GetJMPAbsoluteAddress(unsigned char *jmp) {
	static unsigned char REL_JMP = 0xE9;
	if (*jmp == REL_JMP) {
		uint32_t next_instr = reinterpret_cast<uint32_t>(jmp + 5);
		uint32_t rel_addr = *reinterpret_cast<uint32_t*>(jmp + 1);
		uint32_t abs_addr = rel_addr + next_instr;
		return reinterpret_cast<void*>(abs_addr);
	} 
	return 0;
}

static std::string GetDllName(void *symbol) {
	char module[FILENAME_MAX] = "";
	#ifdef WIN32
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(symbol, &mbi, sizeof(mbi));
		GetModuleFileName((HMODULE)mbi.AllocationBase, module, FILENAME_MAX);
	#else
		Dl_info info;
		dladdr(symbol, &info);
		strcpy(module, info.dli_fname);
	#endif
	return boost::filesystem::basename(module);
}

static const char *GetNativeName(AMX *amx, cell index) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(
		hdr->natives + reinterpret_cast<int32_t>(amx->base));
	AMX_FUNCSTUBNT *libraries = reinterpret_cast<AMX_FUNCSTUBNT*>(
		hdr->libraries + reinterpret_cast<int32_t>(amx->base));

	int numNatives = (hdr->libraries - hdr->natives) / hdr->defsize;

	if (index < 0 || index >= numNatives) {
		return "<unknown>";
	}

	return reinterpret_cast<char*>(natives[index].nameofs +
		reinterpret_cast<int32_t>(hdr));
}

void Crashdetect::ReportCrash() {
	// Check if the last native call succeeded
	if (!nativeCallStack_.empty()) {
		instances[nativeCallStack_.top().GetAmx()]->HandleCrash();
	} else {
		// Server/plugin internal error, we don't know the reason
		logprintf("The server has crashed due to an unknown error");
	}
}

static int AMXAPI AmxDebug(AMX *amx) {
	return ::instances[amx]->AmxDebug();
}

static int AMXAPI AmxCallback(AMX *amx, cell index, cell *result, cell *params) {
	return ::instances[amx]->AmxCallback(index, result, params);
}

static int AMXAPI AmxExec(AMX *amx, cell *retval, int index) {
	return ::instances[amx]->AmxExec(retval, index);
}

std::stack<Crashdetect::CSEntry> Crashdetect::publicCallStack_;
std::stack<Crashdetect::CSEntry> Crashdetect::nativeCallStack_;

bool Crashdetect::errorCaught_ = false;

Crashdetect::Crashdetect(AMX *amx) 
	: amx_(amx)
	, amxhdr_(reinterpret_cast<AMX_HEADER*>(amx->base))
{
	// Try to determine .amx file name.
	amxFileName_ = FindAmxFilePath(amx_);
	if (!amxFileName_.empty()) {
		uint16_t flags;
		amx_Flags(amx_, &flags);
		if ((flags & AMX_FLAG_DEBUG) != 0) {
			debugInfo_.Load(amxFileName_);
		}
	}

	// Disable overriding of SYSREQ.C opcodes by SYSREQ.D
	amx_->sysreq_d = 0;

	// Remember previously set callback and debug hook
	prevDebugHook_ = amx_->debug;
	prevCallback_ = amx_->callback;
}

int Crashdetect::AmxDebug() {
	if (prevDebugHook_ != 0) {
		return prevDebugHook_(amx_);
	}
	return AMX_ERR_NONE;
}

int Crashdetect::AmxExec(cell *retval, int index) {
	publicCallStack_.push(CSEntry(amx_, index));

	int retcode = ::amx_Exec(amx_, retval, index);
	if (retcode != AMX_ERR_NONE && !errorCaught_) {
		amx_Error(amx_, index, retcode);		
	} else {
		errorCaught_ = false;
	}

	assert(publicCallStack_.top().GetAmx() == amx_);
	assert(publicCallStack_.top().GetIndex() == index);
	publicCallStack_.pop();	

	return retcode;
}

int Crashdetect::AmxCallback(cell index, cell *result, cell *params) {
	nativeCallStack_.push(CSEntry(amx_, index));

	// Reset error
	amx_->error = AMX_ERR_NONE;

	// Call any previously set callback (amx_Callback by default)
	prevCallback_(amx_, index, result, params);	

	// Check if the AMX_ERR_NATIVE error is set
	if (amx_->error == AMX_ERR_NATIVE) {
		HandleNativeError(index);
	}

	// Reset error again
	amx_->error = AMX_ERR_NONE;

	assert(nativeCallStack_.top().GetAmx() == amx_);
	assert(nativeCallStack_.top().GetIndex() == index);
	nativeCallStack_.pop();	

	return AMX_ERR_NONE;
}

void Crashdetect::HandleNativeError(int index) {
	if (debugInfo_.IsLoaded()) {
		logprintf("Script[%s]: Native function %s() called at line %ld in '%s' has failed",
			amxFileName_.c_str(), GetNativeName(amx_, index), 
			debugInfo_.GetLineNumber(amx_->cip), 
			debugInfo_.GetFileName(amx_->cip).c_str()
		);				
	} else {
		logprintf("Script[%d]: Native function %s() called at address 0x%X has failed",
			amxFileName_.c_str(), GetNativeName(amx_, index), amx_->cip);
	}
	PrintCallStack();
}

void Crashdetect::HandleRuntimeError(int index, int error) {
	Crashdetect::errorCaught_ = true;
	if (error == AMX_ERR_INDEX && index == AMX_EXEC_GDK) {
		// Fail silently as this public doesn't really exist
		error = AMX_ERR_NONE;
	} else {
		if (!debugInfo_.IsLoaded() || amx_->cip == 0) {
			char entryPoint[33];
			// Get entry point name
			if (index == AMX_EXEC_MAIN) {
				strcpy(entryPoint, "main");
			} else {
				if (index < 0 || amx_GetPublic(amx_, index, entryPoint) != AMX_ERR_NONE) {
					strcpy(entryPoint, "<unknown function>");
				}
			}
			logprintf("Script[%s]: During execution of %s():", 
				amxFileName_.c_str(), entryPoint);
		} else {
			logprintf("Script[%s]: In file '%s' at line %ld:", 
				amxFileName_.c_str(), 
				debugInfo_.GetFileName(amx_->cip).c_str(),
				debugInfo_.GetLineNumber(amx_->cip)					
			);
		}
		logprintf("Script[%s]: Run time error %d: \"%s\"", 
			amxFileName_.c_str(), error, aux_StrError(error));
		logprintf("Error information:");
		switch (error) {
		case AMX_ERR_BOUNDS: {
			ucell bound = *(reinterpret_cast<cell*>(amx_->cip + amx_->base + amxhdr_->cod) - 1);
			ucell index = amx_->pri;
			logprintf("  Array max index is %d but accessing an element at %d", bound, index);
			break;
		}
		case AMX_ERR_NOTFOUND: {
			logprintf("  The following natives were not registered:");
			AMX_FUNCSTUBNT *natives = 
				reinterpret_cast<AMX_FUNCSTUBNT*>(amx_->base + amxhdr_->natives);
			int numNatives = 0;
			amx_NumNatives(amx_, &numNatives);
			for (int i = 0; i < numNatives; ++i) {
				if (natives[i].address == 0) {
					char *name = reinterpret_cast<char*>(natives[i].nameofs + amx_->base);
					logprintf("    %s", name);
				}
			}
			break;
		}
		case AMX_ERR_STACKERR:
			logprintf("  Stack index (STK) is 0x%X, heap index (HEA) is 0x%X", amx_->stk, amx_->hea); 
			break;
		case AMX_ERR_STACKLOW:
			logprintf("  Stack index (STK) is 0x%X, stack top (STP) is 0x%X", amx_->stk, amx_->stp);
			break;
		case AMX_ERR_HEAPLOW:
			logprintf("  Heap index (HEA) is 0x%X, heap bottom (HLW) is 0x%X", amx_->hea, amx_->hlw);
			break;
		case AMX_ERR_INVINSTR: {
			cell opcode = *(reinterpret_cast<cell*>(amx_->cip + amx_->base + amxhdr_->cod) - 1);
			logprintf("  Invalid opcode 0x%X at address 0x%X", opcode , amx_->cip - sizeof(cell));
			break;
		}
		default:
			logprintf("  No details available");
			break;
		}
		PrintCallStack();
	}
}

void Crashdetect::HandleCrash() {
	logprintf("The server has crashed executing '%s'", amxFileName_.c_str());
	PrintCallStack();
}

void Crashdetect::PrintCallStack() const {
	logprintf("Call stack (most recent call first):");	

	if (!nativeCallStack_.empty()) {
		assert(amx_ == nativeCallStack_.top().GetAmx());
		if (debugInfo_.IsLoaded()) {
			logprintf("  File '%s', line %ld", 
				debugInfo_.GetFileName(amx_->cip).c_str(),
				debugInfo_.GetLineNumber(amx_->cip));
			logprintf("    native %s()", 
				GetNativeName(nativeCallStack_.top().GetAmx(), nativeCallStack_.top().GetIndex()));
		} else {
			logprintf("  0x%08X => native %s()", amx_->cip - sizeof(cell), 
				GetNativeName(nativeCallStack_.top().GetAmx(), nativeCallStack_.top().GetIndex()));
		}
	}

	AMXCallStack callStack(amx_, debugInfo_);

	std::vector<AMXStackFrame> frames = callStack.GetFrames();
	if (frames.size() == 0) {
		logprintf("  Stack corrupted!");
		return;
	}
	
	for (std::vector<AMXStackFrame>::const_iterator iterator = frames.begin(); 
			iterator != frames.end(); ++iterator) 
	{	
		const AMXStackFrame &frame = *iterator;

		if (debugInfo_.IsLoaded()) {
			std::string function = frame.GetFunctionName();
			if (!frame.OK() && function.empty()) {
				logprintf("  Stack corrupted!");
				break;
			}
			if (frame.GetCallAddress() != 0) {
				if (!frame.OK()) {
					logprintf("  Stack corrupted!");
					break;
				}
				logprintf("  File '%s', line %d", frame.GetSourceFileName().c_str(), frame.GetLineNumber());;
			} else {
				// Entry point
				logprintf("  File '%s'", frame.GetSourceFileName().c_str());;
			}
			logprintf("    %s", frame.GetFunctionPrototype().c_str());				
		} else {
			if (frame.GetCallAddress() != 0) {
				if (!frame.OK()) {
					logprintf("  Stack corrupted!");
					break;
				}
				if (frame.IsPublic()) {
					logprintf("  0x%08X => public %s()", 
						frame.GetCallAddress(), frame.GetFunctionName().c_str());
				} else {
					logprintf("  0x%08X => 0x%08x()", 
						frame.GetCallAddress(), frame.GetFunctionAddress());
				}
			} else {				
				char name[32];
				if (!publicCallStack_.empty() 
						&& amx_GetPublic(amx_, publicCallStack_.top().GetIndex(), name) == AMX_ERR_NONE) {
					logprintf("  0x???????? => public %s()", name);
				} else if (!publicCallStack_.empty()
						&& publicCallStack_.top().GetIndex() == AMX_EXEC_MAIN) {
					logprintf("  0x???????? => main()");
				} else {
					logprintf("  0x???????? => 0x????????()");				
				}
				break;
			}
		}
	}
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppPluginData) {
	logprintf = (logprintf_t)ppPluginData[PLUGIN_DATA_LOGPRINTF];

	// Hook amx_Exec to catch execution errors.
	// But first make sure it's not already hooked by someone else.
	void *amx_Exec_ptr = ((void**)ppPluginData[PLUGIN_DATA_AMX_EXPORTS])[PLUGIN_AMX_EXPORT_Exec];
	void *funAddr = GetJMPAbsoluteAddress(reinterpret_cast<unsigned char*>(amx_Exec_ptr));
	if (funAddr != 0) {
		std::string module = GetDllName(funAddr);
		if (!module.empty() && module != "samp-server" && module != "samp03svr") {
			logprintf("  crashdetect must be loaded before %s", module.c_str());
			return false;
		}
	} 
	new JumpX86(amx_Exec_ptr, (void*)AmxExec);

	// Set crash handler
	Crash::SetHandler(Crashdetect::ReportCrash);
	Crash::EnableMiniDump(true);

	logprintf("  crashdetect v"VERSION_STRING" is OK.");
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
	logprintf("Unloaded");
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
	::instances[amx].reset(new Crashdetect(amx));
	amx_SetDebugHook(amx, AmxDebug);
	amx_SetCallback(amx, AmxCallback);
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
	::instances.erase(amx);
	return AMX_ERR_NONE;
}
