#pragma once

namespace model {

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

    void promote(PieceType newType) { type_ = newType; }

    void setCooldown(int ms) { cooldown_remaining_ms_ = ms; }
    void decreaseCooldown(int ms) {
        cooldown_remaining_ms_ = (cooldown_remaining_ms_ > ms) ? (cooldown_remaining_ms_ - ms) : 0;
    }
    int cooldownRemainingMs() const { return cooldown_remaining_ms_; }

private:
    PieceType type_ = PieceType::Empty;
    Color color_ = Color::White;
    int cooldown_remaining_ms_ = 0;
};

}  // namespace model
