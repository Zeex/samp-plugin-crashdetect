#include <crashdetect>
#include <test>

main() {
	f();
}

f() {
	error();
}

error() {
	#emit halt 1
}

public OnRuntimeError(code, &bool:suppress) {
	PrintAmxBacktrace();
	suppress = true;
	TestExit();
}
