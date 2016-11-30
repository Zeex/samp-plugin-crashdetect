// Copyright (c) 2013-2015 Zeex
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

#include "crashdetect.h"
#include "natives.h"
#include "os.h"

namespace {

// native PrintAmxBacktrace();
cell AMX_NATIVE_CALL PrintBacktrace(AMX *amx, cell *params) {
  CrashDetect::PrintAMXBacktrace();
  return 1;
}

// native PrintNativeBacktrace();
cell AMX_NATIVE_CALL PrintNativeBacktrace(AMX *amx, cell *params) {
  CrashDetect::PrintNativeBacktrace(os::Context());
  return 1;
}

// native GetAmxBacktrace(string[], size = sizeof(string));
cell AMX_NATIVE_CALL GetBacktrace(AMX *amx, cell *params) {
  cell string = params[1];
  cell size = params[2];

  cell *string_ptr;
  if (amx_GetAddr(amx, string, &string_ptr) == AMX_ERR_NONE) {
    std::stringstream stream;
    CrashDetect::PrintAMXBacktrace(stream);
    return amx_SetString(string_ptr, stream.str().c_str(),
                         0, 0, size) == AMX_ERR_NONE;
  }

  return 0;
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

const AMX_NATIVE_INFO natives[] = {
  {"PrintBacktrace",       PrintBacktrace},
  {"PrintNativeBacktrace", PrintNativeBacktrace},
  {"GetBacktrace",         GetBacktrace},
  {"GetNativeBacktrace",   GetNativeBacktrace},
  // Backwards compatibility:
  {"PrintAmxBacktrace",    PrintBacktrace},
  {"GetAmxBacktrace",      GetBacktrace}
};

} // anonymous namespace

int RegisterNatives(AMX *amx) {
  std::size_t num_natives = sizeof(natives) / sizeof(AMX_NATIVE_INFO);
  return amx_Register(amx, natives, static_cast<int>(num_natives));
}
