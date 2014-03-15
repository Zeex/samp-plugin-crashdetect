#include <a_samp>
#include <test>

public out_of_bounds_pos();
public out_of_bounds_neg();

main() {
	CallLocalFunction("out_of_bounds_pos", "");
	CallLocalFunction("out_of_bounds_neg", "");
	TestExit();
}

public out_of_bounds_pos()
{
	new a[1];
	new i = 100;
	return a[i];
}

public out_of_bounds_neg()
{
	new a[5];
	new i = -100;
	return a[i];
}
