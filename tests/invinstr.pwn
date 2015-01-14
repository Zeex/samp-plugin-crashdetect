#include <a_samp>
#include "test"

main() {
	f();
	TestExit();
}

f() {
	#emit lctrl 6
	#emit add.c 0xfffffffc
	#emit stor.s.pri 4

	// Invalid opcode 0xfffffffc at address XXX
}
