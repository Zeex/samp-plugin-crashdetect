#include <test>

#pragma dynamic 1000

public test();

main() {
	CallLocalFunction("test");
	TestExit();
}

public test() {
	#emit lctrl 3
	#emit const.alt 1
	#emit add
	#emit sctrl 4
	print("Don't remove this message");
}
