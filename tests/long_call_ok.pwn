// FLAGS: -d3
// OUTPUT: Start
// OUTPUT: 1000

#include "test"

main() {
	print("Start");

	new x = 0;

	for (new i = 0; i < 1000; i++) {
		x += floatround(floatlog(10, 10));
	}

	printf("%d", x);
}
