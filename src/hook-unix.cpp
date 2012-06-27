#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#include "hook.h"

void Hook::Unprotect(void *address, int size) {
	// Both address and size must be multiples of page size...
	size_t pagesize = getpagesize();
	size_t where = ((reinterpret_cast<uint32_t>(address) / pagesize) * pagesize);
	size_t count = (size / pagesize) * pagesize + pagesize * 2;
	mprotect(reinterpret_cast<void*>(where), count, PROT_READ | PROT_WRITE | PROT_EXEC);
}
