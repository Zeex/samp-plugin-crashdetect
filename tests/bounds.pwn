// FLAGS: -d3
// OUTPUT: \[debug\] Run time error 4: "Array index out of bounds"
// OUTPUT: \[debug\]  Attempted to read/write array element at index 100 in array of size 1
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 000000d8 in public out_of_bounds_pos \(\) at .*bounds\.pwn:29
// OUTPUT: \[debug\] #1 native CallLocalFunction \(\) in plugin-runner(\.exe)?
// OUTPUT: \[debug\] #2 00000038 in main \(\) at .*bounds\.pwn:21
// OUTPUT: \[debug\] Run time error 4: "Array index out of bounds"
// OUTPUT: \[debug\]  Attempted to read/write array element at negative index -100
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 0000015c in public out_of_bounds_neg \(\) at .*bounds\.pwn:36
// OUTPUT: \[debug\] #1 native CallLocalFunction \(\) in plugin-runner(\.exe)?
// OUTPUT: \[debug\] #2 0000006c in main \(\) at .*bounds\.pwn:22

#include "test"

public out_of_bounds_pos();
public out_of_bounds_neg();

main() {
	CallLocalFunction("out_of_bounds_pos", "");
	CallLocalFunction("out_of_bounds_neg", "");
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
