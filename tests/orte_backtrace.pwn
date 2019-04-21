// FLAGS: -d3
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 native PrintBacktrace \(\) in crashdetect\.(dll|so)
// OUTPUT: \[debug\] #1 00000084 in public OnRuntimeError \(code=1, &bool:suppress=@00000000 false\) at .*orte_backtrace\.pwn:26
// OUTPUT: \[debug\] #2 00000058 in error \(\) at .*orte_backtrace\.pwn:22
// OUTPUT: \[debug\] #3 00000048 in f \(\) at .*orte_backtrace\.pwn:18
// OUTPUT: \[debug\] #4 00000024 in main \(\) at .*orte_backtrace\.pwn:14
// OUTPUT: Error while executing main: Forced exit \(1\)

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
}
