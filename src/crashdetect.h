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

#ifndef CRASHDETECT_H
#define CRASHDETECT_H

#include <stack>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "amxdebuginfo.h"
#include "configreader.h"

// Compatibility with GDK
#define AMX_EXEC_GDK (-10) 

class AMXStackFrame;

class crashdetect {
public:	
	// SA-MP plugin acrhitecture, called from plugin.cpp
	static bool Load(void **ppPluginData);
	static void Unload();
	static int AmxLoad(AMX *amx);
	static int AmxUnload(AMX *amx);

	// Gets a "crashdetect" instance mapped to a AMX or creates a new one.
	static crashdetect *GetInstance(AMX *amx);

	// AMX API hooks.
	static int AMXAPI AmxDebug(AMX *amx);
	static int AMXAPI AmxCallback(AMX *amx, cell index, cell *result, cell *params);
	static int AMXAPI AmxExec(AMX *amx, cell *retval, int index);

	// Global event handlers.
	static void Crash();
	static void RuntimeError(AMX *amx, cell index, int error);
	static void Interrupt();
	static void ExitOnError();	

	// Instance-specific event handlers, called by the global ones (see above).
	void HandleCrash();
	void HandleNativeError(int index);
	void HandleRuntimeError(int index, int error);
	void HandleInterrupt();

	// Called by the global hooks.
	int HandleAmxDebug();
	int HandleAmxCallback(cell index, cell *result, cell *params);
	int HandleAmxExec(cell *retval, int index);

	// Prints the call stack.
	void PrintBacktrace() const;

private:
	explicit crashdetect(AMX *amx);

private:
	// The corresponding AMX instance and its header.
	AMX         *amx_;
	AMX_HEADER  *amxhdr_;

	// AMX debugging information (symbols, file names, line numbers, etc).
	AMXDebugInfo debugInfo_;

	// The path to the AMX file.
	std::string amxPath_;
	// The name of the AMX file + .amx extension.
	std::string amxName_;

	// Old AMX callback and debug hook.
	AMX_CALLBACK prevCallback_;
	AMX_DEBUG    prevDebugHook_;	

	// This class represents both native and public function calls.
	class NativePublicCall {
	public:
		enum Type {
			NATIVE,
			PUBLIC
		};

		NativePublicCall(Type type, AMX *amx, cell index, cell frm, cell cip)
			: type_(type), amx_(amx), index_(index), frm_(frm), cip_(cip) {}

		Type type() const 
			{ return type_; }
		AMX *amx() const 
			{ return amx_; }
		cell index() const 
			{ return index_; }
		cell frm() const 
			{ return frm_; }
		cell cip() const
			{ return cip_; }

	private:
		Type  type_;
		AMX   *amx_;
		cell  frm_;
		cell  cip_;
		cell  index_;
	};

	// The native/public call stack.
	static std::stack<NativePublicCall> npCalls_;

	// This variable is set to true when an AMX runtime error 
	// occurs to prvent double report in certain cases.
	static bool errorCaught_;

	// The server config (server.cfg).
	static ConfigReader serverCfg;

	// AMX* <=> crashdetect*
	static boost::unordered_map<AMX*, boost::shared_ptr<crashdetect> > instances_;
};

#endif // !CRASHDETECT_H
