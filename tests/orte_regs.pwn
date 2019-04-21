// FLAGS: -d3
// OUTPUT: \[debug\] Run time error 4: "Array index out of bounds"
// OUTPUT: \[debug\]  Attempted to read/write array element at index 100500 in array of size 1
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 [0-9a-fA-F]+ in main \(\) at .*orte_regs\.pwn:14
// OUTPUT: Error while executing main: Array index out of bounds \(4\)

#include <crashdetect>
#include "test"

main() {
	new i = 100500;
	new a[1];
	printf("%d", a[i]);
}

public OnRuntimeError(code, &bool:suppress) {
	return 1;
}
