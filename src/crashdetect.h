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

class AMXStackFrame;

class crashdetect {
public:	
	static bool Load(void **ppPluginData);
	static void Unload();
	static int AmxLoad(AMX *amx);
	static int AmxUnload(AMX *amx);

	static crashdetect *GetInstance(AMX *amx);

	static int AMXAPI AmxDebug(AMX *amx);
	static int AMXAPI AmxCallback(AMX *amx, cell index, cell *result, cell *params);
	static int AMXAPI AmxExec(AMX *amx, cell *retval, int index);

	static void Crash();
	static void RuntimeError(AMX *amx, cell index, int error);
	static void Interrupt();
	static void ExitOnError();	

	void HandleCrash();
	void HandleRuntimeError(int index, int error);
	void HandleInterrupt();

	int HandleAmxDebug();
	int HandleAmxCallback(cell index, cell *result, cell *params);
	int HandleAmxExec(cell *retval, int index);

	void PrintBacktrace() const;

private:
	explicit crashdetect(AMX *amx);

	void StackPush(cell value) const;
	void StackPop(int ncells) const;

	AMX         *amx_;
	AMX_HEADER  *amxhdr_;

	AMXDebugInfo debugInfo_;

	std::string amxPath_;
	std::string amxName_;

	AMX_CALLBACK prevCallback_;
	AMX_DEBUG    prevDebugHook_;

	class NPCall {
	public:
		enum Type { NATIVE, PUBLIC };

		NPCall(Type type, AMX *amx, cell index, cell frm, cell cip)
			: type_(type), amx_(amx), index_(index), frm_(frm), cip_(cip) {}

		inline Type type() const { return type_; }
		inline AMX *amx() const { return amx_; }
		inline cell index() const { return index_; }
		inline cell frm() const { return frm_; }
		inline cell cip() const { return cip_; }

	private:
		Type type_;
		AMX  *amx_;
		cell frm_;
		cell cip_;
		cell index_;
	};

	static std::stack<NPCall> npCalls_;
	static bool errorCaught_;
	static ConfigReader serverCfg;
	static boost::unordered_map<AMX*, boost::shared_ptr<crashdetect> > instances_;
};

#endif // !CRASHDETECT_H
