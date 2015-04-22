// FLAGS: -d3
// OUTPUT: \[debug\] Run time error 27: "General error \(unknown or unspecific error\)"
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 00000248 in end \(\) at .*ref_args\.pwn:51
// OUTPUT: \[debug\] #1 00000230 in f3 \(&Float:f=@00003fc4 1\.50000\) at .*ref_args\.pwn:47
// OUTPUT: \[debug\] #2 000001d0 in f2 \(&bool:b1=@00003fe0 true, &bool:b2=@00003fdc false\) at .*ref_args\.pwn:42
// OUTPUT: \[debug\] #3 00000174 in f1 \(&x=@00003ff4 123\) at .*ref_args\.pwn:36
// OUTPUT: \[debug\] #4 000000fc in begin \(\) at .*ref_args\.pwn:29
// OUTPUT: \[debug\] #5 000000b0 in public test \(\) at .*ref_args\.pwn:24
// OUTPUT: \[debug\] #6 native CallLocalFunction \(\) from (samp03svr|samp-server\.exe)
// OUTPUT: \[debug\] #7 00000070 in main \(\) at .*ref_args\.pwn:19

#include <a_samp>
#include "test"

forward test();

main() {
	CallLocalFunction("test", "");
	TestExit();
}

public test() {
	begin();
}

begin() {
	new _x = 123;
	f1(_x);
}

f1(&x) {
	new bool:_b1 = true;
	new bool:_b2 = false;
	f2(_b1, _b2);
	return x;
}

f2(&bool:b1, &bool:b2) {
	new Float:_f = 1.5;
	f3(_f);
	return b1 && b2;
}

f3(&Float:f) {
	end();
	return _:f;
}

end() {
	#emit halt 27
}
