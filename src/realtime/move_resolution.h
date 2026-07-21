#pragma once

#include "../model/piece.h"
#include "../model/position.h"

namespace realtime {

struct CompletedMoveEvent {
    int timestamp_ms = 0;
    model::PieceType piece_type = model::PieceType::Empty;
    model::Color piece_color = model::Color::White;
    model::Position from;
    model::Position to;
    model::PieceType captured_type = model::PieceType::Empty;
    model::Color captured_color = model::Color::White;
};

struct JumpCaptureEvent {
    int timestamp_ms = 0;
    model::Position jump_square;
    model::PieceType jumper_type = model::PieceType::Empty;
    model::Color jumper_color = model::Color::White;
    model::PieceType victim_type = model::PieceType::Empty;
    model::Color victim_color = model::Color::White;
    model::Position victim_from;
    model::Position victim_to;
};

struct MoveStartedEvent {
    int timestamp_ms = 0;
    model::PieceType piece_type = model::PieceType::Empty;
    model::Color piece_color = model::Color::White;
    model::Position from;
    model::Position to;
};

struct JumpStartedEvent {
    int timestamp_ms = 0;
    model::PieceType piece_type = model::PieceType::Empty;
    model::Color piece_color = model::Color::White;
    model::Position square;
};

struct MoveResolution {
    enum class Kind { CompletedMove, JumpCapture };

    Kind kind = Kind::CompletedMove;
    CompletedMoveEvent completed;
    JumpCaptureEvent jump_capture;
};

}  // namespace realtime
