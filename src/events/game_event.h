#pragma once

#include <string>
#include <variant>

#include "../model/piece.h"
#include "../realtime/move_resolution.h"

namespace events {

struct GameStarted {
    std::string white_user;
    std::string black_user;
    int port = 0;
};

struct GameEnded {
    model::Color winner;
};

using GameEvent =
    std::variant<GameStarted, realtime::CompletedMoveEvent, realtime::JumpCaptureEvent, GameEnded,
                 realtime::MoveStartedEvent, realtime::JumpStartedEvent>;

enum class GameEventType {
    GameStarted = 0,
    MoveCompleted = 1,
    JumpCapture = 2,
    GameEnded = 3,
    MoveStarted = 4,
    JumpStarted = 5,
};

inline GameEventType eventType(const GameEvent& event) {
    return static_cast<GameEventType>(event.index());
}

}  // namespace events
