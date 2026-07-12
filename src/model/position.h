#pragma once

namespace model {

struct Position {
    int row;
    int col;

    Position() : row(-1), col(-1) {}
    Position(int r, int c) : row(r), col(c) {}

    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }

    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
};

}  // namespace model
