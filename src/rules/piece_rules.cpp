#include "piece_rules.h"

#include "movement/piece_movement.h"

namespace rules {

bool canMove(const model::Board& board, const model::Position& from, const model::Position& to) {
    if (from == to) {
        return false;
    }

    if (!board.inBounds(from) || !board.inBounds(to)) {
        return false;
    }

    const model::Piece& piece = board.cell(from);
    if (piece.isEmpty()) {
        return false;
    }

    const PieceMovement* movement = movementFor(piece.type());
    if (movement == nullptr || !movement->canMove(board, from, to, piece)) {
        return false;
    }

    const model::Piece& destination = board.cell(to);
    if (!destination.isEmpty() && destination.isFriendly(piece.color())) {
        return false;
    }

    return true;
}

}  // namespace rules
