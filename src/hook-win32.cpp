
#include <windows.h>

#include "hook.h"

void Hook::Unprotect(void *address, int size) {
	DWORD oldProtect;
	VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
}

