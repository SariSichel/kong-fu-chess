#pragma once

#include <string>
#include <variant>

#include "../realtime/move_resolution.h"

namespace events {

struct GameStarted {
    std::string white_user;
    std::string black_user;
    int port = 0;
};

using GameEvent = std::variant<GameStarted, realtime::CompletedMoveEvent, realtime::JumpCaptureEvent>;

enum class GameEventType {
    GameStarted = 0,
    MoveCompleted = 1,
    JumpCapture = 2,
};

inline GameEventType eventType(const GameEvent& event) {
    return static_cast<GameEventType>(event.index());
}

}  // namespace events
