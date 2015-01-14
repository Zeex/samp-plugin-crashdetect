// OUTPUT: CrashDetect is running: yes

#include <a_samp>
#include <crashdetect>
#include "test"

Test() {
	new bool:cd = IsCrashDetectPresent();
	printf("CrashDetect is running: %s", (cd) ? ("yes") : ("no"));
}

main() {
	Test();
	TestExit();
}
