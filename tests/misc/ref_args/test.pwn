#include <a_samp>
#include <test>

forward test();

main() {
	CallLocalFunction("test", "");
	TestExit();
}

public test() {
	begin();
}

begin() {
	new _x = 123;
	f1(_x);
}

f1(&x) {
	new bool:_b1 = true;
	new bool:_b2 = false;
	f2(_b1, _b2);
	return x;
}

f2(&bool:b1, &bool:b2) {
	new Float:_f = 1.5;
	f3(_f);
	return b1 && b2;
}

f3(&Float:f) {
	end();
	return _:f;
}

end() {
	#emit halt 27
}
