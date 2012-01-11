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
#include <stack>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#ifdef _WIN32
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
#include "amxpathfinder.h"
#include "crash.h"
#include "crashdetect.h"
#include "configreader.h"
#include "interrupt.h"
#include "jump-x86.h"
#include "logprintf.h"
#include "plugincommon.h"
#include "version.h"

#include "amx/amx.h"
#include "amx/amxaux.h" // for amx_StrError()

bool 
	crashdetect::errorCaught_ = false;

std::stack<crashdetect::NativePublicCall> 
	crashdetect::npCalls_;

ConfigReader 
	crashdetect::serverCfg("server.cfg");

boost::unordered_map<AMX*, boost::shared_ptr<crashdetect> > 
	crashdetect::instances_;

int AMXAPI crashdetect::AmxDebug(AMX *amx) {
	return instances_[amx]->HandleAmxDebug();
}

int AMXAPI crashdetect::AmxCallback(AMX *amx, cell index, cell *result, cell *params) {
	return instances_[amx]->HandleAmxCallback(index, result, params);
}

int AMXAPI crashdetect::AmxExec(AMX *amx, cell *retval, int index) {
	return instances_[amx]->HandleAmxExec(retval, index);
}

// static
bool crashdetect::Load(void **ppPluginData) {
	// Hook amx_Exec to catch execution errors
	void *amxExecPtr = ((void**)ppPluginData[PLUGIN_DATA_AMX_EXPORTS])[PLUGIN_AMX_EXPORT_Exec];

	// But first make sure it's not already hooked by someone else
	void *funAddr = GetJMPAbsoluteAddress(reinterpret_cast<unsigned char*>(amxExecPtr));
	if (funAddr == 0) {
		new JumpX86(amxExecPtr, (void*)AmxExec);
	} else {
		std::string module = crashdetect::GetModuleNameBySymbol(funAddr);
		if (!module.empty() && module != "samp-server.exe" && module != "samp03svr") {
			logprintf("  crashdetect must be loaded before %s", module.c_str());
			return false;
		}
	}

	// Set crash handler
	Crash::SetHandler(crashdetect::Crash);
	Crash::EnableMiniDump(true);

	// Set Ctrl-C signal handler
	Interrupt::SetHandler(crashdetect::Interrupt);

	logprintf("  crashdetect v"CRASHDETECT_VERSION" is OK.");
	return true;
}

// static
int crashdetect::AmxLoad(AMX *amx) {
	instances_[amx].reset(new crashdetect(amx));
	return AMX_ERR_NONE;
}

// static
int crashdetect::AmxUnload(AMX *amx) {
	instances_.erase(amx);
	return AMX_ERR_NONE;
}

// static 
void *crashdetect::GetJMPAbsoluteAddress(unsigned char *jmp) {
	static unsigned char REL_JMP = 0xE9;
	if (*jmp == REL_JMP) {
		uint32_t next_instr = reinterpret_cast<uint32_t>(jmp + 5);
		uint32_t rel_addr = *reinterpret_cast<uint32_t*>(jmp + 1);
		uint32_t abs_addr = rel_addr + next_instr;
		return reinterpret_cast<void*>(abs_addr);
	} 
	return 0;
}

// static 
std::string crashdetect::GetModuleNameBySymbol(void *symbol) {
	if (symbol == 0) {
		return std::string();
	}
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
	return boost::filesystem::path(module).filename().string();
}

// static
bool crashdetect::GetNativeInfo(AMX *amx, cell index, AMX_NATIVE_INFO &info) {
	AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

	AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(
		hdr->natives + amx->base);
	AMX_FUNCSTUBNT *libraries = reinterpret_cast<AMX_FUNCSTUBNT*>(
		hdr->libraries + amx->base);
	int numNatives = (hdr->libraries - hdr->natives) / hdr->defsize;

	if (index < 0 || index >= numNatives) {
		return false;
	}

	info.func = reinterpret_cast<AMX_NATIVE>(natives[index].address);
	info.name = reinterpret_cast<char*>(natives[index].nameofs +
		reinterpret_cast<int32_t>(hdr));
	return true;
}

// static
const char *crashdetect::GetNativeName(AMX *amx, cell index) {
	AMX_NATIVE_INFO info;
	if (!GetNativeInfo(amx, index, info)) {
		return "<unknown native>";
	}
	return info.name;
}

// static
AMX_NATIVE crashdetect::GetNativeAddress(AMX *amx, cell index) {
	AMX_NATIVE_INFO info;
	if (!GetNativeInfo(amx, index, info)) {
		return 0;
	}
	return info.func;
}

// static
ucell crashdetect::GetPublicAddress(AMX *amx, cell index) {
	AMX_FUNCSTUBNT *publics = reinterpret_cast<AMX_FUNCSTUBNT*>(
		reinterpret_cast<AMX_HEADER*>(amx->base)->publics + amx->base);
	return publics[index].address;
}

// static 
std::string crashdetect::ReadSourceLine(const std::string &filename, long lineNo) {	
	std::ifstream inputFile(filename.c_str());
	if (inputFile.is_open()) {
		std::string line;
		int lineCount = 0;
		while (std::getline(inputFile, line)) {
			if (++lineCount == lineNo) {
				boost::algorithm::trim(line); // strip indents
				return line;
			}
		}
	}
	return std::string();
}

// static
void crashdetect::Crash() {
	// Check if the last native/public call succeeded
	if (!npCalls_.empty()) {
		AMX *amx = npCalls_.top().amx();
		instances_[amx]->HandleCrash();
	} else {
		// Server/plugin internal error (in another thread?)
		logprintf("[debug] The server has crashed due to an unknown error");
	}
}

// static
void crashdetect::RuntimeError(AMX *amx, cell index, int error) {
	instances_[amx]->HandleRuntimeError(index, error);
}

// static
void crashdetect::Interrupt() {	
	if (!npCalls_.empty()) {
		AMX *amx = npCalls_.top().amx();
		instances_[amx]->HandleInterrupt();
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
	if (debugInfo_.IsLoaded()) {
		logprintf("[debug] [%s]: Native function %s() called at line %ld in '%s' has failed",
				amxName_.c_str(), GetNativeName(amx_, index), 
				debugInfo_.GetLineNumber(amx_->cip), 
				debugInfo_.GetFileName(amx_->cip).c_str()
		);				
	} else {
		logprintf("[debug] [%s]: Native function %s() called at address 0x%X has failed",
				amxName_.c_str(), GetNativeName(amx_, index), amx_->cip);
	}
	PrintCallStack();
	ExitOnError();
}

void crashdetect::HandleRuntimeError(int index, int error) {
	crashdetect::errorCaught_ = true;
	if (error == AMX_ERR_INDEX && index == AMX_EXEC_GDK) {
		// Fail silently as this public doesn't really exist
		error = AMX_ERR_NONE;
	} else {
		if (!debugInfo_.IsLoaded() || amx_->cip == 0) {
			char entryPoint[33];
			// GetInstance entry point name
			if (index == AMX_EXEC_MAIN) {
				strcpy(entryPoint, "main");
			} else {
				if (index < 0 || amx_GetPublic(amx_, index, entryPoint) != AMX_ERR_NONE) {
					strcpy(entryPoint, "<unknown function>");
				}
			}
			logprintf("[debug] [%s]: During execution of %s():", 
					amxName_.c_str(), entryPoint);
		} else {
			std::string filename = debugInfo_.GetFileName(amx_->cip);
			long line = debugInfo_.GetLineNumber(amx_->cip);
			logprintf("[debug] [%s]: In file '%s' at line %ld:", 
					amxName_.c_str(), 
					filename.c_str(),
					line
			);
			std::string code = ReadSourceLine(filename, line);
			if (!code.empty()) {
				logprintf("[debug] [%s]: %s", amxName_.c_str(), code.c_str());
			}
		}
		logprintf("[debug] [%s]: Run time error %d: \"%s\"", 
				amxName_.c_str(), error, aux_StrError(error));
		switch (error) {
			case AMX_ERR_BOUNDS: {
				ucell bound = *(reinterpret_cast<cell*>(amx_->cip + amx_->base + amxhdr_->cod) - 1);
				ucell index = amx_->pri;
				logprintf("[debug] [%s]:   Array max. index is %d but accessing an element at %d", 
						amxName_.c_str(), bound, index);
				break;
			}
			case AMX_ERR_NOTFOUND: {
				logprintf("[debug] [%s]:   These natives are not registered:", amxName_.c_str());
				AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(amx_->base + amxhdr_->natives);
				int numNatives = 0;
				amx_NumNatives(amx_, &numNatives);
				for (int i = 0; i < numNatives; ++i) {
					if (natives[i].address == 0) {
						char *name = reinterpret_cast<char*>(natives[i].nameofs + amx_->base);
						logprintf("[debug] [%s]:   %s", amxName_.c_str(), name);
					}
				}
				break;
			}
			case AMX_ERR_STACKERR:
				logprintf("[debug] [%s]:   Stack index (STK) is 0x%X, heap index (HEA) is 0x%X", 
						amxName_.c_str(), amx_->stk, amx_->hea); 
				break;
			case AMX_ERR_STACKLOW:
				logprintf("[debug] [%s]:   Stack index (STK) is 0x%X, stack top (STP) is 0x%X", 
						amxName_.c_str(), amx_->stk, amx_->stp);
				break;
			case AMX_ERR_HEAPLOW:
				logprintf("[debug] [%s]:   Heap index (HEA) is 0x%X, heap bottom (HLW) is 0x%X", 
						amxName_.c_str(), amx_->hea, amx_->hlw);
				break;
			case AMX_ERR_INVINSTR: {
				cell opcode = *(reinterpret_cast<cell*>(amx_->cip + amx_->base + amxhdr_->cod) - 1);
				logprintf("[debug] [%s]:   Invalid opcode 0x%X at address 0x%X", 
						amxName_.c_str(), opcode , amx_->cip - sizeof(cell));
				break;
			}
		}
		PrintCallStack();
		ExitOnError();
	}
}

void crashdetect::HandleCrash() {
	logprintf("[debug] The server has crashed execution '%s'", amxName_.c_str());
	PrintCallStack();
}

void crashdetect::HandleInterrupt() {
	logprintf("[debug] [%s]: Keyboard interrupt", amxName_.c_str());
	PrintCallStack();
}

void crashdetect::PrintCallStack() const {
	if (npCalls_.empty()) 
		return;

	std::stack<NativePublicCall> npCallStack = npCalls_;
	ucell frm = static_cast<ucell>(amx_->frm);

	logprintf("[debug] [%s]: Call stack (most recent call first):", amxName_.c_str());	

	while (!npCallStack.empty()) {
		NativePublicCall call = npCallStack.top();
		AMXDebugInfo debugInfo = instances_[call.amx()]->debugInfo_;
		if (call.type() == NativePublicCall::NATIVE) {			
			std::string module = GetModuleNameBySymbol(
					(void*)GetNativeAddress(call.amx(), call.index()));
			if (debugInfo.IsLoaded()) {
				logprintf("[debug] [%s]:   File '%s', line %ld", amxName_.c_str(),
						debugInfo.GetFileName(call.amx()->cip).c_str(),
						debugInfo.GetLineNumber(call.amx()->cip));
				logprintf("[debug] [%s]:     native %s() from %s", amxName_.c_str(),
						GetNativeName(call.amx(), call.index()), module.c_str());
			} else {
				logprintf("[debug] [%s]:   0x%08X => native %s() from %s", amxName_.c_str(), 
						call.amx()->cip - sizeof(cell), GetNativeName(call.amx(), call.index()), module.c_str());
			}
		} else if (call.type() == NativePublicCall::PUBLIC) {
			AMXCallStack callStack(call.amx(), debugInfo, frm);
			std::vector<AMXStackFrame> frames = callStack.GetFrames();
			if (frames.empty()) {
				logprintf("[debug] [%s]:   Some or all frames are missing", amxName_.c_str());
			}
			if (debugInfo.IsLoaded()) {
				for (std::vector<AMXStackFrame>::const_iterator iterator = frames.begin(); 
					iterator != frames.end() && iterator->GetCallAddress() != 0; ++iterator) 
				{	
					logprintf("[debug] [%s]:   File '%s', line %d", amxName_.c_str(),
							iterator->GetSourceFileName().c_str(), iterator->GetLineNumber());;
					logprintf("[debug] [%s]:     %s", amxName_.c_str(), 
							iterator->GetFunctionPrototype().c_str());
				}
				// Entry point
				AMXStackFrame epFrame = AMXStackFrame(call.amx(), frm, 0, 
						GetPublicAddress(call.amx(), call.index()), debugInfo);
				logprintf("[debug] [%s]:   File '%s'", amxName_.c_str(), epFrame.GetSourceFileName().c_str());
				if (call.index() == AMX_EXEC_MAIN) {
					logprintf("[debug] [%s]:     main()", amxName_.c_str());
				} else if (call.index() >= 0) {					
					logprintf("[debug] [%s]:     %s", amxName_.c_str(), epFrame.GetFunctionPrototype().c_str());
				} else {
					logprintf("[debug] [%s]:     <unknown function>", amxName_.c_str());
				}
			} else {
				for (std::vector<AMXStackFrame>::const_iterator iterator = frames.begin(); 
					iterator != frames.end() && iterator->GetCallAddress() != 0; ++iterator) 
				{	
					if (iterator->IsPublic()) {
						logprintf("[debug] [%s]:   0x%08X => public %s()", amxName_.c_str(),
							iterator->GetCallAddress(), iterator->GetFunctionName().c_str());
					} else {
						logprintf("[debug] [%s]:   0x%08X => 0x%08x()", amxName_.c_str(),
							iterator->GetCallAddress(), iterator->GetFunctionAddress());
					}
				}
				// Entry point
				char name[32];
				if (amx_GetPublic(call.amx(), call.index(), name) == AMX_ERR_NONE) {
					logprintf("[debug] [%s]:   0x???????? => public %s()", amxName_.c_str(), name);
				} else if (call.index() == AMX_EXEC_MAIN) {
					logprintf("[debug] [%s]:   0x???????? => main()", amxName_.c_str());
				} else {
					logprintf("[debug] [%s]:   0x???????? => 0x????????()", amxName_.c_str());				
				}
			}
		} else {
			assert(0 && "Invalid call.type()");
		}
		frm = call.frm();
		npCallStack.pop();
	}
}

