#pragma once

#include "../../model/board.h"
#include "../../model/position.h"

namespace rules {

bool isPathClear(const model::Board& board, const model::Position& from, const model::Position& to);
bool isStraightMove(const model::Position& from, const model::Position& to);
bool isDiagonalMove(const model::Position& from, const model::Position& to);
bool canSlideStraight(const model::Board& board, const model::Position& from, const model::Position& to);
bool canSlideDiagonal(const model::Board& board, const model::Position& from, const model::Position& to);

}  // namespace rules
