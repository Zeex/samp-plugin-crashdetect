// OUTPUT: CrashDetect is running: yes

#include <crashdetect>
#include "test"

Test() {
	new bool:cd = IsCrashDetectPresent();
	printf("CrashDetect is running: %s", (cd) ? ("yes") : ("no"));
}

main() {
	Test();
}
