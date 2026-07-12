#include "board.h"

namespace model {

void Board::clear() {
    grid_.clear();
}

void Board::addRow(std::vector<Piece> row) {
    grid_.push_back(std::move(row));
}

bool Board::inBounds(const Position& pos) const {
    return pos.row >= 0 && pos.col >= 0 &&
           static_cast<size_t>(pos.row) < rows() &&
           static_cast<size_t>(pos.col) < cols();
}

const Piece& Board::cell(const Position& pos) const {
    return grid_[pos.row][pos.col];
}

Piece& Board::cell(const Position& pos) {
    return grid_[pos.row][pos.col];
}

void Board::movePiece(const Position& from, const Position& to) {
    grid_[to.row][to.col] = grid_[from.row][from.col];
    grid_[from.row][from.col] = Piece::empty();
}

void Board::removePieceAt(const Position& pos) {
    grid_[pos.row][pos.col] = Piece::empty();
}

}  // namespace model
