#pragma once

#include "../model/board.h"
#include "../model/position.h"

namespace rules {

bool canMove(const model::Board& board, const model::Position& from, const model::Position& to);

}  // namespace rules
