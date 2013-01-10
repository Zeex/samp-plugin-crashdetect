#include <a_samp>

public out_of_bounds_pos();
public out_of_bounds_neg();

main() {
	CallLocalFunction("out_of_bounds_pos", "");
	CallLocalFunction("out_of_bounds_neg", "");
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
