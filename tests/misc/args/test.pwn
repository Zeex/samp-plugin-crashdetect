#include <a_samp>
#include <test>

main() {
	CallLocalFunction("test", "");
	TestExit();
}

public test() {
	begin();
}

begin() {
	f1(123);
}

f1(x) {
	f2(true, false);
	return x;
}

f2(bool:b1, bool:b2) {
	f3(1.5);
	return b1 && b2;
}

f3(Float:f) {
	f4("hi there");
	return _:f;
}

f4(s[]) {
	new a[] = {3, 2, 1};
	f5(a);
	return s[0];
}

f5(a[3]) {
	new aa[][] = {
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	};
	f6(aa);
	return a[0];
}

enum struct {
	abc
};

f6(aa[3][4]) {
	new e[struct] = { 0xdeadbeef };
	f7(e);
	return aa[0][0];
}

f7(e[struct]) {
	f8(UknownTag:999);
	return e[abc];
}

f8(UknownTag:n) {
	end();
	return _:n;
}

end() {
	#emit halt 27
}
