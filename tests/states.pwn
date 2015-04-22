// FLAGS: -d3
// OUTPUT: Foo:on
// OUTPUT: \[debug\] Run time error 2: "Assertion failed"
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 000001f8 in foo \(x=2\) <Foo:on, undefined> at .*states\.pwn:33
// OUTPUT: \[debug\] #1 0000013c in public test \(\) at .*states\.pwn:23
// OUTPUT: \[debug\] #2 native CallLocalFunction \(\) from (samp03svr|samp-server\.exe)
// OUTPUT: \[debug\] #3 000000dc in main \(\) at .*states\.pwn:16

#include <a_samp>
#include "test"

public test();

main() {
	CallLocalFunction("test", "");
	TestExit();
}

public test() {
	state Foo:on;
	foo(2);
	state Bar:on;
	bar(1);
}

foo(x) <> {
	printf("Foo:??", x);
}

foo(x) <Foo:on, undefined> {
	printf("Foo:on", x);
	#emit halt 2
}

foo() <Foo:off> {
	printf("Foo:off");
}

bar(y) <Bar:on> {
	printf("Bar:on", y);
}

bar(y) <Bar:off> {
	printf("Bar:off", y);
}
