#include <test>

public test();

main() {
	CallLocalFunction("test", "");
	TestExit();
}

public test() {
	function1();
}

function1() {
	new i = 0x12345678;
	#emit load.s.pri i
	#emit stor.s.pri 0
	#emit stor.s.pri 4
	#emit stor.s.pri 8
	#emit stor.s.pri 12
	new a[1];
	a[i] = 0;
}
