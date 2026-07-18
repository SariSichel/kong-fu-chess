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

    void setMoveCooldown(int ms) {
        move_cooldown_remaining_ms_ = ms;
        move_cooldown_total_ms_ = ms;
    }
    void setJumpCooldown(int ms) {
        jump_cooldown_remaining_ms_ = ms;
        jump_cooldown_total_ms_ = ms;
    }
    void decreaseCooldown(int ms) {
        if (move_cooldown_remaining_ms_ > ms) {
            move_cooldown_remaining_ms_ -= ms;
        } else {
            move_cooldown_remaining_ms_ = 0;
        }
        if (jump_cooldown_remaining_ms_ > ms) {
            jump_cooldown_remaining_ms_ -= ms;
        } else {
            jump_cooldown_remaining_ms_ = 0;
        }
    }
    int moveCooldownRemainingMs() const { return move_cooldown_remaining_ms_; }
    int jumpCooldownRemainingMs() const { return jump_cooldown_remaining_ms_; }
    int moveCooldownTotalMs() const { return move_cooldown_total_ms_; }
    int jumpCooldownTotalMs() const { return jump_cooldown_total_ms_; }

    bool hasMoved() const { return has_moved_; }
    void markMoved() { has_moved_ = true; }

private:
    PieceType type_ = PieceType::Empty;
    Color color_ = Color::White;
    int move_cooldown_remaining_ms_ = 0;
    int move_cooldown_total_ms_ = 0;
    int jump_cooldown_remaining_ms_ = 0;
    int jump_cooldown_total_ms_ = 0;
    bool has_moved_ = false;
};

}  // namespace model
