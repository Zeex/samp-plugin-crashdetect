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

#if defined CRASHDETECT_INC
	#endinput
#endif
#define CRASHDETECT_INC

// Backwards compatibility; will be removed in the future.
#define PrintAmxBacktrace PrintBacktrace
#define GetAmxBacktrace GetBacktrace

native PrintBacktrace();
native PrintNativeBacktrace();

native GetBacktrace(string[], size = sizeof(string));
native GetNativeBacktrace(string[], size = sizeof(string));

forward OnRuntimeError(code, &bool:suppress);

stock bool:IsCrashDetectPresent() {
	// 0xFF is roughly flags, but some are mutually exclusive:
	//
	//   1 - Present (read only, write always 0).
	//   2 - long_call_time checks enabled (write ignored when `server.cfg` has `long_call_time 0`).
	//   4 - long_call_time reset to default time (write only, read always 0).
	//   8 - long_call_time restart check from now (write only, read always 0).
	//
	#emit zero.pri
	#emit lctrl 0xFF
	#emit not
	#emit not
	#emit retn
	return false;
}

stock SetCrashDetectLongCallTime(us_time) {
	// Setting this to `0` doesn't change the internal time, merely disables the check.
	#emit load.s.pri us_time
	#emit sctrl 0xFE
}

stock DisableCrashDetectLongCall() {
	#emit const.pri 0
	#emit sctrl 0xFE
}

stock EnableCrashDetectLongCall() {
	#emit const.pri 2
	#emit sctrl 0xFF
}

stock ResetCrashDetectLongCallTime() {
	#emit const.pri 4
	#emit sctrl 0xFF
}

stock RestartCrashDetectLongCall() {
	#emit const.pri 8
	#emit sctrl 0xFF
}

stock bool:IsCrashDetectLongCallEnabled() {
	#emit zero.pri
	#emit lctrl 0xFF
	#emit shr.c.pri 1
	#emit retn
	return false;
}

stock GetCrashDetectLongCallTime() {
	#emit zero.pri
	#emit lctrl 0xFE
	#emit retn
	return 0;
}

// `GetCrashDetectDefaultLongCallTime` is too long.
stock GetCrashDetectDefaultTime() {
	// store the current value
	#emit zero.pri
	#emit lctrl 0xFE
	#emit move.alt
	// reset to and read the default
	#emit const.pri 4
	#emit sctrl 0xFF
	#emit zero.pri
	#emit lctrl 0xFE
	// put the current value back
	#emit xchg
	#emit sctrl 0xFE
	#emit move.pri
	#emit retn
	return 0;
}

