// FLAGS: -d3
// OUTPUT: \[debug\] Run time error 27: "General error \(unknown or unspecific error\)"
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 00000344 in end \(\) at .*args\.pwn:86
// OUTPUT: \[debug\] #1 0000032c in f8 \(UknownTag:n=999\) at .*args\.pwn:82
// OUTPUT: \[debug\] #2 000002fc in f7 \(e\[struct:1\]=@00003f70\) at .*args\.pwn:77
// OUTPUT: \[debug\] #3 000002a8 in f6 \(aa\[3\]\[4\]=@00003f84\) at .*args\.pwn:72
// OUTPUT: \[debug\] #4 00000240 in f5 \(a\[3\]=@00003fd0 ""\) at .*args\.pwn:62
// OUTPUT: \[debug\] #5 000001c4 in f4 \(s\[\]=@00000018 "hi there"\) at .*args\.pwn:52
// OUTPUT: \[debug\] #6 00000168 in f3 \(Float:f=1\.50000\) at .*args\.pwn:46
// OUTPUT: \[debug\] #7 00000104 in f2 \(bool:b1=true, bool:b2=false\) at .*args\.pwn:41
// OUTPUT: \[debug\] #8 000000cc in f1 \(x=123\) at .*args\.pwn:36
// OUTPUT: \[debug\] #9 00000094 in begin \(\) at .*args\.pwn:31
// OUTPUT: \[debug\] #10 00000064 in public test \(\) at .*args\.pwn:27
// OUTPUT: \[debug\] #11 native CallLocalFunction \(\) in plugin-runner(\.exe)?
// OUTPUT: \[debug\] #12 00000038 in main \(\) at .*args\.pwn:23

#include "test"

forward test();

main() {
	CallLocalFunction("test", "");
}

public test() {
	begin();
}

begin() {
	f1(123);
}

f1(x) {
	f2(true, false);
	return x;
}

f2(bool:b1, bool:b2) {
	f3(1.5);
	return b1 && b2;
}

f3(Float:f) {
	f4("hi there");
	return _:f;
}

f4(s[]) {
	new a[] = {3, 2, 1};
	f5(a);
	return s[0];
}

f5(a[3]) {
	new aa[][] = {
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	};
	f6(aa);
	return a[0];
}

enum struct {
	abc
};

f6(aa[3][4]) {
	new e[struct] = { 0xdeadbeef };
	f7(e);
	return aa[0][0];
}

f7(e[struct]) {
	f8(UknownTag:999);
	return e[abc];
}

f8(UknownTag:n) {
	end();
	return _:n;
}

end() {
	#emit halt 27
}
