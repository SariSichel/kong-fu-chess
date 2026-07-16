#pragma once

#include "../../model/board.h"
#include "../../model/piece.h"
#include "../../model/position.h"

namespace rules {

class PieceMovement {
public:
    virtual ~PieceMovement() = default;
    virtual bool canMove(const model::Board& board,
                         const model::Position& from,
                         const model::Position& to,
                         const model::Piece& piece) const = 0;
};

class KingMovement : public PieceMovement {
public:
    bool canMove(const model::Board& board,
                 const model::Position& from,
                 const model::Position& to,
                 const model::Piece& piece) const override;
};

class RookMovement : public PieceMovement {
public:
    bool canMove(const model::Board& board,
                 const model::Position& from,
                 const model::Position& to,
                 const model::Piece& piece) const override;
};

class BishopMovement : public PieceMovement {
public:
    bool canMove(const model::Board& board,
                 const model::Position& from,
                 const model::Position& to,
                 const model::Piece& piece) const override;
};

class QueenMovement : public PieceMovement {
public:
    bool canMove(const model::Board& board,
                 const model::Position& from,
                 const model::Position& to,
                 const model::Piece& piece) const override;
};

class KnightMovement : public PieceMovement {
public:
    bool canMove(const model::Board& board,
                 const model::Position& from,
                 const model::Position& to,
                 const model::Piece& piece) const override;
};

class PawnMovement : public PieceMovement {
public:
    bool canMove(const model::Board& board,
                 const model::Position& from,
                 const model::Position& to,
                 const model::Piece& piece) const override;
};

const PieceMovement* movementFor(model::PieceType type);

}  // namespace rules
