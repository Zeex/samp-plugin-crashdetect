// FLAGS: -d3
// OUTPUT: \[debug\] Run time error 4: "Array index out of bounds"
// OUTPUT: \[debug\]  Attempted to read/write array element at index 100500 in array of size 1
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 [0-9a-fA-F]+ in main \(\) at .*orte_regs\.pwn:15
// OUTPUT: Script\[.*orte_regs\.amx\]: Run time error 4: "Array index out of bounds"

#include <a_samp>
#include <crashdetect>
#include "test"

main() {
	new i = 100500;
	new a[1];
	printf("%d", a[i]);
}

public OnRuntimeError(code, &bool:suppress) {
	TestExit();
	return 1;
}
