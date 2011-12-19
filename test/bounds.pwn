#include <a_samp>

main() {}

public OnGameModeInit()
{
	do_out_of_bounds();
	return 1;
}

stock do_out_of_bounds()
{
	new bla[5];

	new fffuuuu = 0;

	fffuuuu = 100;
	bla[fffuuuu] = 100;

	return bla[fffuuuu];
}
