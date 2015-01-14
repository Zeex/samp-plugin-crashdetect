// OUTPUT: \[debug\] Run time error 4: "Array index out of bounds"
// OUTPUT: \[debug\]  Accessing element at index 305419896 past array upper bound 0
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 0000013c in function1 \(\.\.\. <76354974 variable arguments>\) at .*test\.pwn:22
// OUTPUT: \[debug\] #1 native CallLocalFunction \(\) from (samp03svr|samp-server\.exe)
// OUTPUT: \[debug\] #2 00000070 in main \(\) at .*test\.pwn:6

#include "test"

public test();

main() {
	CallLocalFunction("test", "");
	TestExit();
}

public test() {
	function1();
}

function1() {
	new i = 0x12345678;
	#emit load.s.pri i
	#emit stor.s.pri 0
	#emit stor.s.pri 4
	#emit stor.s.pri 8
	#emit stor.s.pri 12
	new a[1];
	a[i] = 0;
}
