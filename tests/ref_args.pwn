// FLAGS: -d3
// OUTPUT: \[debug\] Run time error 27: "General error \(unknown or unspecific error\)"
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 000001fc in end \(\) at .*ref_args\.pwn:49
// OUTPUT: \[debug\] #1 000001e4 in f3 \(&Float:f=@00003fb0 1\.50000\) at .*ref_args\.pwn:45
// OUTPUT: \[debug\] #2 00000184 in f2 \(&bool:b1=@00003fcc true, &bool:b2=@00003fc8 false\) at .*ref_args\.pwn:40
// OUTPUT: \[debug\] #3 00000128 in f1 \(&x=@00003fe0 123\) at .*ref_args\.pwn:34
// OUTPUT: \[debug\] #4 000000b0 in begin \(\) at .*ref_args\.pwn:27
// OUTPUT: \[debug\] #5 00000064 in public test \(\) at .*ref_args\.pwn:22
// OUTPUT: \[debug\] #6 native CallLocalFunction \(\) in plugin-runner(\.exe)?
// OUTPUT: \[debug\] #7 00000038 in main \(\) at .*ref_args\.pwn:18

#include "test"

forward test();

main() {
	CallLocalFunction("test", "");
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
