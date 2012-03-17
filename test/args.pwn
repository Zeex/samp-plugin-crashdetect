#include <a_samp>

forward call_f1(argument);

main() {
	do_f1("HELL YEAH BABY!");
}

do_f1(text[]) {
	print(text);
	CallLocalFunction("call_f1", "i", 0);
}

public call_f1(argument) {
	f1(argument);
}

f1(i) {
	return f2(i + 100);
}

f2(j) {
	return f3(j + 200);
}

f3(k) {
	new a[5];
	return a[k];
}
