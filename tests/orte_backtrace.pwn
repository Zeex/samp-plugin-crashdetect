// FLAGS: -d3
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 native PrintBacktrace \(\) from crashdetect\.(dll|so)
// OUTPUT: \[debug\] #1 000000b8 in public OnRuntimeError \(code=1, &bool:suppress=@00000014 false\) at .*orte_backtrace\.pwn:26
// OUTPUT: \[debug\] #2 0000008c in error \(\) at .*orte_backtrace\.pwn:22
// OUTPUT: \[debug\] #3 0000007c in f \(\) at .*orte_backtrace\.pwn:18
// OUTPUT: \[debug\] #4 00000058 in main \(\) at .*orte_backtrace\.pwn:14
// OUTPUT: Script\[.*orte_backtrace.amx\]: Run time error 1: "Forced exit"

#include <crashdetect>
#include "test"

main() {
	f();
}

f() {
	error();
}

error() {
	#emit halt 1
}

public OnRuntimeError(code, &bool:suppress) {
	PrintAmxBacktrace();
	suppress = true;
	TestExit();
}
