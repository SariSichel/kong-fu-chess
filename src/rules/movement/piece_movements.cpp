#include "piece_movement.h"

#include "movement_helpers.h"

#include <cstdlib>

namespace rules {

bool KingMovement::canMove(const model::Board& board,
                           const model::Position& from,
                           const model::Position& to,
                           const model::Piece& /*piece*/) const {
    (void)board;
    const int dr = std::abs(to.row - from.row);
    const int dc = std::abs(to.col - from.col);
    return dr <= 1 && dc <= 1 && (dr != 0 || dc != 0);
}

bool RookMovement::canMove(const model::Board& board,
                           const model::Position& from,
                           const model::Position& to,
                           const model::Piece& /*piece*/) const {
    return canSlideStraight(board, from, to);
}

bool BishopMovement::canMove(const model::Board& board,
                             const model::Position& from,
                             const model::Position& to,
                             const model::Piece& /*piece*/) const {
    return canSlideDiagonal(board, from, to);
}

bool QueenMovement::canMove(const model::Board& board,
                            const model::Position& from,
                            const model::Position& to,
                            const model::Piece& /*piece*/) const {
    return canSlideStraight(board, from, to) || canSlideDiagonal(board, from, to);
}

bool KnightMovement::canMove(const model::Board& board,
                             const model::Position& from,
                             const model::Position& to,
                             const model::Piece& /*piece*/) const {
    (void)board;
    const int dr = std::abs(to.row - from.row);
    const int dc = std::abs(to.col - from.col);
    return (dr == 1 && dc == 2) || (dr == 2 && dc == 1);
}

bool PawnMovement::canMove(const model::Board& board,
                           const model::Position& from,
                           const model::Position& to,
                           const model::Piece& piece) const {
    const int dr = to.row - from.row;
    const int dc = to.col - from.col;
    const model::Piece& destination = board.cell(to);
    const int forwardDr = (piece.color() == model::Color::White) ? -1 : 1;

    if (dr == forwardDr && dc == 0) {
        return destination.isEmpty();
    }

    if (dr == 2 * forwardDr && dc == 0) {
        if (piece.hasMoved()) {
            return false;
        }
        const int homeRank = (piece.color() == model::Color::White)
                                 ? static_cast<int>(board.rows()) - 2
                                 : 1;
        if (from.row != homeRank) {
            return false;
        }
        const int intermediateR = from.row + forwardDr;
        return board.cell(model::Position{intermediateR, from.col}).isEmpty() && destination.isEmpty();
    }

    if (dr == forwardDr && std::abs(dc) == 1) {
        return !destination.isEmpty() && !destination.isFriendly(piece.color());
    }

    return false;
}

const PieceMovement* movementFor(model::PieceType type) {
    static const KingMovement king;
    static const QueenMovement queen;
    static const RookMovement rook;
    static const BishopMovement bishop;
    static const KnightMovement knight;
    static const PawnMovement pawn;

    switch (type) {
        case model::PieceType::King:
            return &king;
        case model::PieceType::Queen:
            return &queen;
        case model::PieceType::Rook:
            return &rook;
        case model::PieceType::Bishop:
            return &bishop;
        case model::PieceType::Knight:
            return &knight;
        case model::PieceType::Pawn:
            return &pawn;
        case model::PieceType::Empty:
        default:
            return nullptr;
    }
}

}  // namespace rules
