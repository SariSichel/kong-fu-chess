#pragma once

#include <vector>

#include "piece.h"
#include "position.h"

namespace model {

class Board {
public:
    Board() = default;

    void clear();
    void addRow(std::vector<Piece> row);

    size_t rows() const { return grid_.size(); }
    size_t cols() const { return grid_.empty() ? 0 : grid_[0].size(); }

    bool inBounds(const Position& pos) const;
    const Piece& cell(const Position& pos) const;
    Piece& cell(const Position& pos);
    void movePiece(const Position& from, const Position& to);
    void removePieceAt(const Position& pos);

private:
    std::vector<std::vector<Piece>> grid_;
};

}  // namespace model
