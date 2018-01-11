// Copyright (c) 2011-2015 Zeex
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

#include <sstream>
#include <string>

#include <subhook.h>

#include "amxerror.h"
#include "crashdetect.h"
#include "fileutils.h"
#include "logprintf.h"
#include "natives.h"
#include "os.h"
#include "plugincommon.h"
#include "pluginversion.h"

static SubHook exec_hook;

static int AMXAPI AmxDebug(AMX *amx) {
  return CrashDetect::GetInstance(amx)->HandleAMXDebug();
}

static int AMXAPI AmxCallback(AMX *amx, cell index, cell *result, cell *params) {
  return CrashDetect::GetInstance(amx)->HandleAMXCallback(index, result, params);
}

static int AMXAPI AmxExec(AMX *amx, cell *retval, int index) {
  if (amx->flags & AMX_FLAG_BROWSE) {
    return amx_Exec(amx, retval, index);
  }
  return CrashDetect::GetInstance(amx)->HandleAMXExec(retval, index);
}

static void AMXAPI AmxExecError(AMX *amx, cell index, cell *retval, int error) {
  CrashDetect::GetInstance(amx)->HandleAMXExecError(index, retval, error);
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
  return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
  void **exports = reinterpret_cast<void**>(ppData[PLUGIN_DATA_AMX_EXPORTS]);
  ::logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];

  void *amx_Exec_ptr = exports[PLUGIN_AMX_EXPORT_Exec];
  void *amx_Exec_sub = SubHook::ReadDst(amx_Exec_ptr);

  if (amx_Exec_sub == 0) {
    exec_hook.Install(amx_Exec_ptr, (void*)AmxExec);
  } else {
    std::string module = fileutils::GetFileName(os::GetModuleName(amx_Exec_sub));
    if (!module.empty()) {
      logprintf("  CrashDetect must be loaded before '%s'", module.c_str());
    }
    return false;
  }

  os::SetCrashHandler(CrashDetect::OnCrash);
  os::SetInterruptHandler(CrashDetect::OnInterrupt);

  logprintf("  CrashDetect plugin " PROJECT_VERSION_STRING);
  return true;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
  CrashDetect::CreateInstance(amx)->Load();

  amx_SetDebugHook(amx, AmxDebug);
  amx_SetCallback(amx, AmxCallback);
  amx_SetExecErrorHandler(amx, AmxExecError);

  RegisterNatives(amx);
  return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
  CrashDetect::GetInstance(amx)->Unload();
  CrashDetect::DestroyInstance(amx);
  return AMX_ERR_NONE;
}
