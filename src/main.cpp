#include <iostream>

#include "board.h"

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    return Board::run(std::cin, std::cout);
}
