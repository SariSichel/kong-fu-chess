#include "piece_rules.h"

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

bool canKingMove(const model::Board& board, const model::Position& from, const model::Position& to) {
    (void)board;
    const int dr = std::abs(to.row - from.row);
    const int dc = std::abs(to.col - from.col);
    return dr <= 1 && dc <= 1 && (dr != 0 || dc != 0);
}

bool canRookMove(const model::Board& board, const model::Position& from, const model::Position& to) {
    if (from.row != to.row && from.col != to.col) {
        return false;
    }
    return isPathClear(board, from, to);
}

bool canBishopMove(const model::Board& board, const model::Position& from, const model::Position& to) {
    const int dr = to.row - from.row;
    const int dc = to.col - from.col;
    if (std::abs(dr) != std::abs(dc)) {
        return false;
    }
    return isPathClear(board, from, to);
}

bool canQueenMove(const model::Board& board, const model::Position& from, const model::Position& to) {
    return canRookMove(board, from, to) || canBishopMove(board, from, to);
}

bool canKnightMove(const model::Board& board, const model::Position& from, const model::Position& to) {
    (void)board;
    const int dr = std::abs(to.row - from.row);
    const int dc = std::abs(to.col - from.col);
    return (dr == 1 && dc == 2) || (dr == 2 && dc == 1);
}

bool canPawnMove(const model::Board& board, const model::Position& from, const model::Position& to) {
    const int dr = to.row - from.row;
    const int dc = to.col - from.col;
    const model::Piece& piece = board.cell(from);
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

    bool movementOk = false;
    switch (piece.type()) {
        case model::PieceType::King:
            movementOk = canKingMove(board, from, to);
            break;
        case model::PieceType::Rook:
            movementOk = canRookMove(board, from, to);
            break;
        case model::PieceType::Bishop:
            movementOk = canBishopMove(board, from, to);
            break;
        case model::PieceType::Queen:
            movementOk = canQueenMove(board, from, to);
            break;
        case model::PieceType::Knight:
            movementOk = canKnightMove(board, from, to);
            break;
        case model::PieceType::Pawn:
            movementOk = canPawnMove(board, from, to);
            break;
        case model::PieceType::Empty:
        default:
            return false;
    }

    if (!movementOk) {
        return false;
    }

    const model::Piece& destination = board.cell(to);
    if (!destination.isEmpty() && destination.isFriendly(piece.color())) {
        return false;
    }

    return true;
}

}  // namespace rules
