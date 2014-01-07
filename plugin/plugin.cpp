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

#include <sstream>
#include <string>

#include "amxerror.h"
#include "compiler.h"
#include "crashdetect.h"
#include "fileutils.h"
#include "hook.h"
#include "logprintf.h"
#include "os.h"
#include "plugincommon.h"
#include "pluginversion.h"
#include "thread.h"
#include "updater.h"
#include "version.h"

static int AMXAPI AmxCallback(AMX *amx, cell index, cell *result, cell *params) {
  return CrashDetect::GetInstance(amx)->DoAmxCallback(index, result, params);
}

static int AMXAPI AmxExec(AMX *amx, cell *retval, int index) {
  if (amx->flags & AMX_FLAG_BROWSE) {
    return amx_Exec(amx, retval, index);
  }
  return CrashDetect::GetInstance(amx)->DoAmxExec(retval, index);
}

static void AMXAPI AmxExecError(AMX *amx, cell index, cell *retval, int error) {
  CrashDetect::GetInstance(amx)->HandleExecError(index, retval, error);
}

namespace natives {

// native GetAmxBacktrace(string[], size = sizeof(string));
cell AMX_NATIVE_CALL GetAmxBacktrace(AMX *amx, cell *params) {
  cell string = params[1];
  cell size = params[2];

  cell *string_ptr;
  if (amx_GetAddr(amx, string, &string_ptr) == AMX_ERR_NONE) {
    std::stringstream stream;
    CrashDetect::PrintAmxBacktrace(stream);
    return amx_SetString(string_ptr, stream.str().c_str(),
                         0, 0, size) == AMX_ERR_NONE;
  }

  return 0;
}

// native PrintAmxBacktrace();
cell AMX_NATIVE_CALL PrintAmxBacktrace(AMX *amx, cell *params) {
  CrashDetect::PrintAmxBacktrace();
  return 1;
}

// native GetNativeBacktrace(string[], size = sizeof(string));
cell AMX_NATIVE_CALL GetNativeBacktrace(AMX *amx, cell *params) {
  cell string = params[1];
  cell size = params[2];

  cell *string_ptr;
  if (amx_GetAddr(amx, string, &string_ptr) == AMX_ERR_NONE) {
    std::stringstream stream;
    CrashDetect::PrintNativeBacktrace(stream, 0);
    return amx_SetString(string_ptr, stream.str().c_str(),
                         0, 0, size) == AMX_ERR_NONE;
  }

  return 0;
}

// native PrintNativeBacktrace();
cell AMX_NATIVE_CALL PrintNativeBacktrace(AMX *amx, cell *params) {
  CrashDetect::PrintNativeBacktrace(0);
  return 1;
}

const AMX_NATIVE_INFO list[] = {
  {"GetAmxBacktrace",      natives::GetAmxBacktrace},
  {"PrintAmxBacktrace",    natives::PrintAmxBacktrace},
  {"GetNativeBacktrace",   natives::GetNativeBacktrace},
  {"PrintNativeBacktrace", natives::PrintNativeBacktrace},
  {0,                      0}
};

} // namespace natives

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
  return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
  void **exports = reinterpret_cast<void**>(ppData[PLUGIN_DATA_AMX_EXPORTS]);
  ::logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];

  void *amx_Exec_ptr = exports[PLUGIN_AMX_EXPORT_Exec];
  void *amx_Exec_sub = Hook::GetTargetAddress(reinterpret_cast<unsigned char*>(amx_Exec_ptr));

  if (amx_Exec_sub == 0) {
    new Hook(amx_Exec_ptr, (void*)AmxExec);
  } else {
    std::string module = fileutils::GetFileName(os::GetModuleName(amx_Exec_sub));
    if (!module.empty()) {
      logprintf("  CrashDetect must be loaded before '%s'", module.c_str());
    }
    return false;
  }

  os::SetExceptionHandler(CrashDetect::OnException);
  os::SetInterruptHandler(CrashDetect::OnInterrupt);

  Updater::InitiateVersionFetch();

  logprintf("  CrashDetect v"PROJECT_VERSION_STRING" is OK.");
  return true;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
  int error = CrashDetect::CreateInstance(amx)->Load();
  if (error == AMX_ERR_NONE) {
    amx_SetCallback(amx, AmxCallback);
    amx_SetExecErrorHandler(amx, AmxExecError);
    return amx_Register(amx, natives::list, -1);
  }
  return error;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
  int error = CrashDetect::GetInstance(amx)->Unload();
  CrashDetect::DestroyInstance(amx);
  return error;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick() {
  if (Updater::version_fetched()) {
    Version latest_version = Updater::latest_version();
    Version current_version(PROJECT_VERSION_STRING);
    if (current_version < latest_version) {
      logprintf("CrashDetect %s is released!",
                latest_version.AsString().c_str());
      logprintf("Get it here: "
                "https://github.com/Zeex/samp-plugin-crashdetect/releases/tag/v%s",
                latest_version.AsString().c_str());
    }
  }
}
