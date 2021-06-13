// FLAGS: -d3
// OUTPUT: Start
// OUTPUT: \[debug\] Long callback execution detected \(hang or performance issue\)
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 00000[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F] in main \(\) at .*long_call_error\.pwn:(15|16|17)
// OUTPUT: 100000

#include "test"

main() {
	print("Start");

	new x = 0;

	for (new i = 0; i < 100000; i++) {
		x += floatround(floatlog(10, 10));
	}

	printf("%d", x);
}
