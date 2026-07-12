#pragma once

#include "../model/piece.h"
#include "../model/position.h"

namespace realtime {

enum class MotionType {
    Move,
    Jump
};

struct Motion {
    int motion_id;
    model::Position source;
    model::Position destination;
    MotionType type;
    int started_at_ms;
    int duration_ms;
    model::PieceType piece_type;
    model::Color piece_color;

    Motion(int motion_id,
           model::Position source,
           model::Position destination,
           MotionType type,
           int started_at_ms,
           int duration_ms,
           model::PieceType piece_type,
           model::Color piece_color)
        : motion_id(motion_id),
          source(source),
          destination(destination),
          type(type),
          started_at_ms(started_at_ms),
          duration_ms(duration_ms),
          piece_type(piece_type),
          piece_color(piece_color) {}

    bool isExpired(int current_time_ms) const {
        return current_time_ms >= started_at_ms + duration_ms;
    }
};

}  // namespace realtime
