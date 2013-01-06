#if defined __clang__
	#pragma clang push
	#pragma clang diagnostic ignored "-Wignored-attributes"
#elif defined __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wattributes"
#endif

#include <amx/amx.h>
#include <amx/amxaux.h>
#include <amx/amxdbg.h>

#if defined __clang_
	#pragma clang pop
#elif defined __GNUC__
	#pragma GCC pop
#endif
