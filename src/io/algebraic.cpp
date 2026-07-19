#include "algebraic.h"

namespace io {

std::string positionToAlgebraic(const model::Position& pos) {
    if (pos.row < 0 || pos.col < 0) {
        return "??";
    }

    const char file = static_cast<char>('a' + pos.col);
    const int rank = 8 - pos.row;
    return std::string(1, file) + std::to_string(rank);
}

model::Position algebraicToPosition(const std::string& square) {
    if (square.size() != 2) {
        return model::Position(-1, -1);
    }

    const char file = square[0];
    const char rank_char = square[1];
    if (file < 'a' || file > 'h' || rank_char < '1' || rank_char > '8') {
        return model::Position(-1, -1);
    }

    const int col = file - 'a';
    const int rank = rank_char - '0';
    const int row = 8 - rank;
    return model::Position(row, col);
}

}  // namespace io
