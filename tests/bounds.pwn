// FLAGS: -d3
// OUTPUT: \[debug\] Run time error 4: "Array index out of bounds"
// OUTPUT: \[debug\]  Attempted to read/write array element at index 100 in array of size 1
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 00000124 in public out_of_bounds_pos \(\) at .*bounds\.pwn:31
// OUTPUT: \[debug\] #1 native CallLocalFunction \(\) from (samp03svr|samp-server\.exe)
// OUTPUT: \[debug\] #2 00000070 in main \(\) at .*bounds\.pwn:22
// OUTPUT: \[debug\] Run time error 4: "Array index out of bounds"
// OUTPUT: \[debug\]  Attempted to read/write array element at negative index -100
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 000001a8 in public out_of_bounds_neg \(\) at .*bounds\.pwn:38
// OUTPUT: \[debug\] #1 native CallLocalFunction \(\) from (samp03svr|samp-server\.exe)
// OUTPUT: \[debug\] #2 000000a4 in main \(\) at .*bounds\.pwn:23

#include <a_samp>
#include "test"

public out_of_bounds_pos();
public out_of_bounds_neg();

main() {
	CallLocalFunction("out_of_bounds_pos", "");
	CallLocalFunction("out_of_bounds_neg", "");
	TestExit();
}

public out_of_bounds_pos()
{
	new a[1];
	new i = 100;
	return a[i];
}

public out_of_bounds_neg()
{
	new a[5];
	new i = -100;
	return a[i];
}
