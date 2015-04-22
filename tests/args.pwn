// FLAGS: -d3
// OUTPUT: \[debug\] Run time error 27: "General error \(unknown or unspecific error\)"
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 00000390 in end \(\) at .*args\.pwn:86
// OUTPUT: \[debug\] #1 00000378 in f8 \(UknownTag:n=999\) at .*args\.pwn:82
// OUTPUT: \[debug\] #2 00000348 in f7 \(e\[struct:1\]=@00003f84\) at .*args\.pwn:77
// OUTPUT: \[debug\] #3 000002f4 in f6 \(aa\[3\]\[4\]=@00003f98\) at .*args\.pwn:72
// OUTPUT: \[debug\] #4 0000028c in f5 \(a\[3\]=@00003fe4 ""\) at .*args\.pwn:62
// OUTPUT: \[debug\] #5 00000210 in f4 \(s\[\]=@0000002c "hi there"\) at .*args\.pwn:52
// OUTPUT: \[debug\] #6 000001b4 in f3 \(Float:f=1\.50000\) at .*args\.pwn:46
// OUTPUT: \[debug\] #7 00000150 in f2 \(bool:b1=true, bool:b2=false\) at .*args\.pwn:41
// OUTPUT: \[debug\] #8 00000118 in f1 \(x=123\) at .*args\.pwn:36
// OUTPUT: \[debug\] #9 000000e0 in begin \(\) at .*args\.pwn:31
// OUTPUT: \[debug\] #10 000000b0 in public test \(\) at .*args\.pwn:27
// OUTPUT: \[debug\] #11 native CallLocalFunction \(\) from (samp03svr|samp-server\.exe)
// OUTPUT: \[debug\] #12 00000070 in main \(\) at .*args\.pwn:22

#include <a_samp>
#include "test"

main() {
	CallLocalFunction("test", "");
	TestExit();
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
