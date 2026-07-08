#ifndef PIECE_H
#define PIECE_H

enum class Color {
    White,
    Black
};

enum class PieceType {
    Empty,
    King,
    Queen,
    Rook,
    Bishop,
    Knight,
    Pawn
};

class Piece {
public:
    Piece() = default;
    Piece(PieceType type, Color color) : type_(type), color_(color) {}

    static Piece empty() { return Piece(); }

    bool isEmpty() const { return type_ == PieceType::Empty; }
    bool isFriendly(Color color) const { return !isEmpty() && color_ == color; }

    PieceType type() const { return type_; }
    Color color() const { return color_; }

private:
    PieceType type_ = PieceType::Empty;
    Color color_ = Color::White;
};

#endif
