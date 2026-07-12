#include "rule_engine.h"

namespace rules {

bool RuleEngine::validateMove(const model::Board& board, const model::Position& from, const model::Position& to) {
    if (!board.inBounds(from) || !board.inBounds(to)) {
        return false;
    }

    const model::Piece& piece = board.cell(from);
    if (piece.isEmpty()) {
        return false;
    }

    if (piece.cooldownRemainingMs() > 0) {
        return false;
    }

    return canMove(board, from, to);
}

}  // namespace rules
