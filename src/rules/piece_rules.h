#pragma once

#include "../model/board.h"
#include "../model/position.h"

namespace rules {

bool canMove(const model::Board& board, const model::Position& from, const model::Position& to);
bool canKingMove(const model::Board& board, const model::Position& from, const model::Position& to);
bool canRookMove(const model::Board& board, const model::Position& from, const model::Position& to);
bool canBishopMove(const model::Board& board, const model::Position& from, const model::Position& to);
bool canQueenMove(const model::Board& board, const model::Position& from, const model::Position& to);
bool canKnightMove(const model::Board& board, const model::Position& from, const model::Position& to);
bool canPawnMove(const model::Board& board, const model::Position& from, const model::Position& to);
bool isPathClear(const model::Board& board, const model::Position& from, const model::Position& to);

}  // namespace rules
