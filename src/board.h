#ifndef BOARD_H
#define BOARD_H

#include <iosfwd>
#include <vector>

#include "piece.h"

class Board {
public:
    Board() = default;

    static int run(std::istream& in, std::ostream& out);

    void clear();
    void addRow(std::vector<Piece> row);

    size_t rows() const { return grid_.size(); }
    size_t cols() const { return grid_.empty() ? 0 : grid_[0].size(); }

    bool inBounds(int row, int col) const;
    const Piece& cell(int row, int col) const;
    Piece& cell(int row, int col);
    void movePiece(int fromR, int fromC, int toR, int toC);
    void arrivePiece(int fromR, int fromC, int toR, int toC);
    void cancelMoveAt(int fromR, int fromC);
    void removePieceAt(int fromR, int fromC);
    bool canMove(int fromR, int fromC, int toR, int toC) const;

private:
    //move direction
    static int sign(int delta);

    bool isPathClear(int fromR, int fromC, int toR, int toC) const;
    bool canKingMove(int fromR, int fromC, int toR, int toC) const;
    bool canRookMove(int fromR, int fromC, int toR, int toC) const;
    bool canBishopMove(int fromR, int fromC, int toR, int toC) const;
    bool canQueenMove(int fromR, int fromC, int toR, int toC) const;
    bool canKnightMove(int fromR, int fromC, int toR, int toC) const;
    bool canPawnMove(int fromR, int fromC, int toR, int toC) const;
    //board grid
    std::vector<std::vector<Piece>> grid_;
};

#endif
