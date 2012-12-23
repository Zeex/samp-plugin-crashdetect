// Copyright (c) 2012, Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cstdarg>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <amx/amx.h> // for uint32_t

#include "logprintf.h"

logprintf_t logprintf;

void vlogprintf(const char *format, std::va_list args) {
	int nargs = 0;

	// Since the number of arguments isn't known we can only guess...
	//
	// Best method I came up with is to count number of format specifiers in the
	// format string. But there's one issue with this - generally printf()-like
	// functions can take arguments of various size - from 1 byte to 8. Things
	// that are smaller than 4 bytes are actually pushed as dwords anyway so
	// they are fine. Bigger values are pushed as sequences of two dwords (and
	// read by printf accordingly if the size matches the speficier).
	//
	// Because I have no idea whether logprintf() accepts the latter ones this
	// code will assume that each of the unnamed arguments is exactly one mahine
	// word (32 bits) in size so things should go smooth (otherwise it may not
	// work).
	std::size_t length = strlen(format);
	for (std::size_t i = 0; i < length; i++) {
		if (format[i] == '%' && format[i + 1] != '%') {
			nargs++;
		}
	}

	uint32_t *oldEsp;
	uint32_t *newEsp;
	int i = 0;

	// Since this line new local variables must not be allocated!
	// ----------------------------------------------------------

	#if defined __GNUC__
		__asm__ __volatile__ ("movl %%esp, %0" : "=r"(oldEsp));
	#elif defined _MSC_VER
		__asm mov dword ptr [oldEsp], esp
	#else
		#error Unsupported compiler
	#endif

	oldEsp -= 4; /* for saved registers */
	newEsp = oldEsp - (nargs + 1); /* args + format */

	newEsp[0] = reinterpret_cast<uint32_t>(format);
	for (i = 0; i < nargs; i++) {
		newEsp[i + 1] = va_arg(args, uint32_t);
	}

	#if defined __GNUC__
		__asm__ __volatile__ (
			"push %%eax\n\t"
			"push %%ecx\n\t"
			"push %%edx\n\t"
			"push %%esi\n\t"
			"movl %0, %%esi\n\t"
			"movl %1, %%esp\n\t"
			"call *%2\n\t"
			"movl %%esi, %%esp\n\t"
			"pop %%esi\n\t"
			"pop %%edx\n\t"
			"pop %%ecx\n\t"
			"pop %%eax\n\t"
		: : "r"(oldEsp), "r"(newEsp), "r"(::logprintf));
	#elif defined _MSC_VER
		__asm {
			push eax
			push ecx
			push edx
			push esi
			mov esi, dword ptr [oldEsp]
			mov esp, dword ptr [newEsp]
			mov eax, dword ptr [logprintf]
			call eax
			mov esp, esi
			pop esi
			pop edx
			pop ecx
			pop eax
		}
	#else
		#error Unsupported compiler
	#endif
}
