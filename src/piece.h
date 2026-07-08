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

enum class PieceMovementState {
    Idle,
    Moving,
    Airborne
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

    PieceMovementState movementState() const { return movementState_; }
    bool isMoving() const { return movementState_ == PieceMovementState::Moving; }
    bool isAirborne() const { return movementState_ == PieceMovementState::Airborne; }

    // A piece is "busy" while a timed move or jump is in flight. Busy pieces
    // can neither start a new move nor start a jump.
    bool isBusy() const { return isMoving() || isAirborne(); }

    // Jump legality: the piece must exist (a captured piece becomes Empty and
    // is therefore ineligible) and must be idle (not moving, not already airborne).
    bool canJump() const { return !isEmpty() && movementState_ == PieceMovementState::Idle; }

    int currentRow() const { return currentRow_; }
    int currentCol() const { return currentCol_; }
    int destinationRow() const { return destinationRow_; }
    int destinationCol() const { return destinationCol_; }

    void beginMove(int fromRow, int fromCol, int toRow, int toCol) {
        movementState_ = PieceMovementState::Moving;
        currentRow_ = fromRow;
        currentCol_ = fromCol;
        destinationRow_ = toRow;
        destinationCol_ = toCol;
    }

    void finishMove() {
        movementState_ = PieceMovementState::Idle;
        currentRow_ = destinationRow_;
        currentCol_ = destinationCol_;
        destinationRow_ = -1;
        destinationCol_ = -1;
    }

    void cancelMove() {
        movementState_ = PieceMovementState::Idle;
        destinationRow_ = -1;
        destinationCol_ = -1;
    }

    // The piece stays on its own cell for the entire jump; it has no destination.
    void beginJump(int row, int col) {
        movementState_ = PieceMovementState::Airborne;
        currentRow_ = row;
        currentCol_ = col;
        destinationRow_ = -1;
        destinationCol_ = -1;
    }

    // Land in place: back to Idle on the same cell it jumped from.
    void finishJump() {
        movementState_ = PieceMovementState::Idle;
        destinationRow_ = -1;
        destinationCol_ = -1;
    }

    void promote(PieceType newType) { type_ = newType; }

private:
    PieceType type_ = PieceType::Empty;
    Color color_ = Color::White;
    PieceMovementState movementState_ = PieceMovementState::Idle;
    int currentRow_ = -1;
    int currentCol_ = -1;
    int destinationRow_ = -1;
    int destinationCol_ = -1;
};

#endif
