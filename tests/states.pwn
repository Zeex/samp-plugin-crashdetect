// FLAGS: -d3
// OUTPUT: Foo:on
// OUTPUT: \[debug\] Run time error 2: "Assertion failed"
// OUTPUT: \[debug\] AMX backtrace:
// OUTPUT: \[debug\] #0 000001ac in foo \(x=2\) <Foo:on, undefined> at .*states\.pwn:31
// OUTPUT: \[debug\] #1 000000f0 in public test \(\) at .*states\.pwn:21
// OUTPUT: \[debug\] #2 native CallLocalFunction \(\) in plugin-runner(\.exe)?
// OUTPUT: \[debug\] #3 000000a4 in main \(\) at .*states\.pwn:15

#include "test"

public test();

main() {
	CallLocalFunction("test", "");
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
