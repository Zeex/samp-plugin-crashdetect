#include <a_samp>

main() {
    function1();
}

function1() {
    function2();
}

function2() {
    new buf[10];
    fread(File:123, buf);
}

