#include <test>

Test() {
	new cd;
	#emit zero.pri
	#emit lctrl 0xFF
	#emit stor.s.pri cd
	printf("CrashDetect is running: %s", (cd)? ("yes"): ("no"));
}

main() {
	Test();
	TestExit();
}
