#include <iostream>

#include "command_processor.h"

int main() {
    return CommandProcessor::run(std::cin, std::cout);
}
