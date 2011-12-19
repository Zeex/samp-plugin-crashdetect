// Crashdetect plugin test -  Stack Underflow

#include <a_samp>

#pragma dynamic 1000

main() {
	#emit lctrl 3
	#emit const.alt 1
	#emit add
	#emit sctrl 4
	printf("Hello!");
}
