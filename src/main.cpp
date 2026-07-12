//git link:
//https://github.com/SariSichel/kong-fu-chess/tree/main
#include <iostream>

#include "texttests/script_runner.h"

int main() {
    texttests::ScriptRunner runner;
    runner.run(std::cin, std::cout);
    return 0;
}
