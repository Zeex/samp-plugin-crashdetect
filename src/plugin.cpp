// Copyright (c) 2011-2021 Zeex
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

#include <functional>
#include <string>
#ifdef _WIN32
  #include <windows.h>
#else
  #include <stdio.h>
#endif
#include <subhook.h>
#include "amxpathfinder.h"
#include "crashdetect.h"
#include "fileutils.h"
#include "logprintf.h"
#include "natives.h"
#include "options.h"
#include "os.h"
#include "plugincommon.h"
#include "pluginversion.h"
#include "stringutils.h"

namespace {

std::vector<std::function<void()>> unload_callbacks;

subhook::Hook exec_hook;
subhook::Hook open_file_hook;
std::string last_opened_amx_file_name;

#ifdef _WIN32
  HANDLE WINAPI CreateFileAHook(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
  {
    subhook::ScopedHookRemove _(&open_file_hook);
    const char *ext = fileutils::GetFileExtensionPtr(lpFileName);
    if (ext != nullptr && stringutils::CompareIgnoreCase(ext, "amx") == 0) {
      last_opened_amx_file_name = lpFileName;
    }
    return CreateFileA(
      lpFileName,
      dwDesiredAccess,
      dwShareMode,
      lpSecurityAttributes,
      dwCreationDisposition,
      dwFlagsAndAttributes,
      hTemplateFile);
  }
#else
  FILE *FopenHook(const char *filename, const char *mode) {
    subhook::ScopedHookRemove _(&open_file_hook);
    const char *ext = fileutils::GetFileExtensionPtr(filename);
    if (ext != nullptr && stringutils::CompareIgnoreCase(ext, "amx") == 0) {
      last_opened_amx_file_name = filename;
    }
    return fopen(filename, mode);
  }
#endif

int AMXAPI OnDebugHook(AMX *amx) {
  return CrashDetect::GetHandler(amx)->OnDebugHook();
}

int AMXAPI OnCallback(AMX *amx, cell index, cell *result, cell *params) {
  CrashDetect *handler = CrashDetect::GetHandler(amx);
  return handler->OnCallback(index, result, params);
}

int AMXAPI OnExec(AMX *amx, cell *retval, int index) {
  if (amx->flags & AMX_FLAG_BROWSE) {
    return amx_Exec(amx, retval, index);
  }
  CrashDetect *handler = CrashDetect::GetHandler(amx);
  if (handler == nullptr) {
    return amx_Exec(amx, retval, index);
  }
  return handler->OnExec(retval, index);
}

int AMXAPI OnExecError(AMX *amx, cell index, cell *retval, int error) {
  CrashDetect *handler = CrashDetect::GetHandler(amx);
  return handler->OnExecError(index, retval, error);
}

int AMXAPI OnLongCallRequest(AMX *amx, int option, int value) {
  CrashDetect *handler = CrashDetect::GetHandler(amx);
  return handler->OnLongCallRequest(option, value);
}

int AMXAPI OnAddressNaughtRequest(AMX *amx, int option) {
  CrashDetect *handler = CrashDetect::GetHandler(amx);
  return handler->OnAddressNaughtRequest(option);
}

} // anonymous namespace

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
  return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
  void **exports = reinterpret_cast<void**>(ppData[PLUGIN_DATA_AMX_EXPORTS]);
  ::logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];

  void *amx_Exec_ptr = exports[PLUGIN_AMX_EXPORT_Exec];
  void *amx_Exec_sub = subhook::ReadHookDst(amx_Exec_ptr);

  if (amx_Exec_sub == nullptr) {
    exec_hook.Install(amx_Exec_ptr, (void*)OnExec);
    unload_callbacks.push_back([]() {
      exec_hook.Remove();
    });
  } else {
    std::string module =
      fileutils::GetFileName(os::GetModuleName(amx_Exec_sub));
    if (!module.empty()) {
      logprintf("  CrashDetect must be loaded before '%s'", module.c_str());
    }
    return false;
  }

  #if _WIN32
    open_file_hook.Install((void*)CreateFileA, (void*)CreateFileAHook);
  #else
    open_file_hook.Install((void*)fopen, (void*)FopenHook);
  #endif
  unload_callbacks.push_back([]() {
    open_file_hook.Remove();
  });

  AMXPathFinder::shared().AddSearchPath("gamemodes");
  AMXPathFinder::shared().AddSearchPath("filterscripts");

  const char *amx_path_var = getenv("AMX_PATH");
  if (amx_path_var != nullptr) {
    stringutils::SplitString(
      amx_path_var,
      fileutils::kNativePathListSepChar,
      std::bind1st(std::mem_fun(&AMXPathFinder::AddSearchPath),
                   &AMXPathFinder::shared()));
  }

  os::SetCrashHandler(CrashDetect::OnCrash);
  os::SetInterruptHandler(CrashDetect::OnInterrupt);
  CrashDetect::PluginLoad();

  logprintf("  CrashDetect plugin " PLUGIN_VERSION_STRING);
  return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
  CrashDetect::PluginUnload();

  for (auto &callback : unload_callbacks) {
    callback();
  }
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
  if (!last_opened_amx_file_name.empty()) {
    AMXPathFinder::shared().AddKnownFile(amx, last_opened_amx_file_name);
  }

  CrashDetect *handler = CrashDetect::CreateHandler(amx);
  handler->Load();

  amx_SetDebugHook(amx, OnDebugHook);
  amx_SetCallback(amx, OnCallback);

  static AMX_EXT_HOOKS ext_hooks = {
    OnExecError,
    OnLongCallRequest,
    OnAddressNaughtRequest
  };
  amx_SetExtHooks(amx, &ext_hooks);

  RegisterNatives(amx);
  return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
  CrashDetect::GetHandler(amx)->Unload();
  CrashDetect::DestroyHandler(amx);
  return AMX_ERR_NONE;
}
