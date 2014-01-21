#include <test>

forward ExitTimer();

main() {
	SetTimer("ExitTimer", 1, false);
	#emit halt 1
}

public ExitTimer() {
	TestExit();
}
