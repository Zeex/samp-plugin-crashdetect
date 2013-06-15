#include <a_samp>
#include <test>

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
