#include "board.h"

#include <cstdlib>

#include "command_processor.h"

void Board::clear() {
    grid_.clear();
}

void Board::addRow(std::vector<Piece> row) {
    grid_.push_back(std::move(row));
}

bool Board::inBounds(int row, int col) const {
    return row >= 0 && col >= 0 && static_cast<size_t>(row) < rows() &&
           static_cast<size_t>(col) < cols();
}

const Piece& Board::cell(int row, int col) const {
    return grid_[row][col];
}

Piece& Board::cell(int row, int col) {
    return grid_[row][col];
}

void Board::movePiece(int fromR, int fromC, int toR, int toC) {
    grid_[toR][toC] = grid_[fromR][fromC];
    grid_[fromR][fromC] = Piece::empty();
}

void Board::arrivePiece(int fromR, int fromC, int toR, int toC) {
    grid_[toR][toC] = grid_[fromR][fromC];
    grid_[fromR][fromC] = Piece::empty();
    grid_[toR][toC].finishMove();
}

int Board::sign(int delta) {
    if (delta > 0) {
        return 1;
    }
    if (delta < 0) {
        return -1;
    }
    return 0;
}

bool Board::isPathClear(int fromR, int fromC, int toR, int toC) const {
    const int stepR = sign(toR - fromR);
    const int stepC = sign(toC - fromC);
    int r = fromR + stepR;
    int c = fromC + stepC;

    while (r != toR || c != toC) {
        if (!cell(r, c).isEmpty()) {
            return false;
        }
        r += stepR;
        c += stepC;
    }

    return true;
}

bool Board::canKingMove(int fromR, int fromC, int toR, int toC) const {
    const int dr = std::abs(toR - fromR);
    const int dc = std::abs(toC - fromC);
    return dr <= 1 && dc <= 1 && (dr != 0 || dc != 0);
}

bool Board::canRookMove(int fromR, int fromC, int toR, int toC) const {
    if (fromR != toR && fromC != toC) {
        return false;
    }
    return isPathClear(fromR, fromC, toR, toC);
}

bool Board::canBishopMove(int fromR, int fromC, int toR, int toC) const {
    const int dr = toR - fromR;
    const int dc = toC - fromC;
    if (std::abs(dr) != std::abs(dc)) {
        return false;
    }
    return isPathClear(fromR, fromC, toR, toC);
}

bool Board::canQueenMove(int fromR, int fromC, int toR, int toC) const {
    return canRookMove(fromR, fromC, toR, toC) ||
           canBishopMove(fromR, fromC, toR, toC);
}

bool Board::canKnightMove(int fromR, int fromC, int toR, int toC) const {
    const int dr = std::abs(toR - fromR);
    const int dc = std::abs(toC - fromC);
    return (dr == 1 && dc == 2) || (dr == 2 && dc == 1);
}

bool Board::canPawnMove(int fromR, int fromC, int toR, int toC) const {
    const int dr = toR - fromR;
    const int dc = toC - fromC;
    const Piece& piece = cell(fromR, fromC);
    const Piece& destination = cell(toR, toC);
    // White moves up (decreasing row); Black moves down (increasing row)
    const int forwardDr = (piece.color() == Color::White) ? -1 : 1;
    // Forward: exactly 1 cell, same column, destination must be empty
    if (dr == forwardDr && dc == 0) {
        return destination.isEmpty();
    }
    // Diagonal capture: exactly 1 cell diagonally forward, enemy on destination
    if (dr == forwardDr && std::abs(dc) == 1) {
        return !destination.isEmpty() && !destination.isFriendly(piece.color());
    }
    // Everything else is illegal (including 2-cell forward, backward, sideways)
    return false;
}

bool Board::canMove(int fromR, int fromC, int toR, int toC) const {
    if (fromR == toR && fromC == toC) {
        return false;
    }

    if (!inBounds(fromR, fromC) || !inBounds(toR, toC)) {
        return false;
    }

    const Piece& piece = cell(fromR, fromC);
    if (piece.isEmpty()) {
        return false;
    }

    bool movementOk = false;
    switch (piece.type()) {
        case PieceType::King:
            movementOk = canKingMove(fromR, fromC, toR, toC);
            break;
        case PieceType::Rook:
            movementOk = canRookMove(fromR, fromC, toR, toC);
            break;
        case PieceType::Bishop:
            movementOk = canBishopMove(fromR, fromC, toR, toC);
            break;
        case PieceType::Queen:
            movementOk = canQueenMove(fromR, fromC, toR, toC);
            break;
        case PieceType::Knight:
            movementOk = canKnightMove(fromR, fromC, toR, toC);
            break;
        case PieceType::Pawn:
            movementOk = canPawnMove(fromR, fromC, toR, toC);
            break;
        case PieceType::Empty:
        default:
            return false;
    }

    if (!movementOk) {
        return false;
    }

    const Piece& destination = cell(toR, toC);
    if (!destination.isEmpty() && destination.isFriendly(piece.color())) {
        return false;
    }

    return true;
}

int Board::run(std::istream& in, std::ostream& out) {
    return CommandProcessor::run(in, out);
}
