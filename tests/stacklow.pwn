// OUTPUT: Don't remove this message
// OUTPUT: \[debug\] Run time error 7: "Stack underflow"
// OUTPUT: \[debug\]  Stack pointer \(STK\) is 0x102D, stack top \(STP\) is 0x102C
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 000000cc in public test \(\.\.\. <4 variable arguments>\) at .*test\.pwn:17
// OUTPUT: \[debug\] #1 native CallLocalFunction \(\) from (samp03svr|samp-server.exe)
// OUTPUT: \[debug\] #2 00000064 in main \(\.\.\. <6400 variable arguments>\) at .*test\.pwn:8
// OUTPUT: \[debug\] Run time error 5: "Invalid memory access"
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 00000084 in main \(\) at .*test\.pwn:9
// OUTPUT: Script\[.*test\.amx\]: Run time error 5: "Invalid memory access"

#include "test"

#pragma dynamic 1000

public test();

main() {
	CallLocalFunction("test");
	TestExit();
}

public test() {
	#emit lctrl 3
	#emit const.alt 1
	#emit add
	#emit sctrl 4
	print("Don't remove this message");
}
