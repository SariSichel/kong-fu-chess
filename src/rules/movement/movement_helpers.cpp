#include "movement_helpers.h"

#include <cstdlib>

namespace {

int sign(int delta) {
    if (delta > 0) {
        return 1;
    }
    if (delta < 0) {
        return -1;
    }
    return 0;
}

}  // namespace

namespace rules {

bool isPathClear(const model::Board& board, const model::Position& from, const model::Position& to) {
    const int stepR = sign(to.row - from.row);
    const int stepC = sign(to.col - from.col);
    int r = from.row + stepR;
    int c = from.col + stepC;

    while (r != to.row || c != to.col) {
        if (!board.cell(model::Position{r, c}).isEmpty()) {
            return false;
        }
        r += stepR;
        c += stepC;
    }

    return true;
}

bool isStraightMove(const model::Position& from, const model::Position& to) {
    return from.row == to.row || from.col == to.col;
}

bool isDiagonalMove(const model::Position& from, const model::Position& to) {
    const int dr = to.row - from.row;
    const int dc = to.col - from.col;
    return std::abs(dr) == std::abs(dc) && dr != 0;
}

bool canSlideStraight(const model::Board& board, const model::Position& from, const model::Position& to) {
    return isStraightMove(from, to) && isPathClear(board, from, to);
}

bool canSlideDiagonal(const model::Board& board, const model::Position& from, const model::Position& to) {
    return isDiagonalMove(from, to) && isPathClear(board, from, to);
}

}  // namespace rules
