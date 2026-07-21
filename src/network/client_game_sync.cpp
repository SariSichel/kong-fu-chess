#include "client_game_sync.h"

#include "../constants.h"
#include "../engine/game_engine.h"
#include "ws_protocol.h"

namespace network {

namespace {

constexpr int kSyncFrameMs = 16;

void fastForwardMove(engine::GameEngine& game_engine, const model::Position& from,
                     const model::Position& to) {
    if (!game_engine.requestMove(from, to)) {
        return;
    }

    for (int i = 0; i < 32; ++i) {
        game_engine.advanceTime(kSyncFrameMs);
    }
}

void fastForwardJump(engine::GameEngine& game_engine, const model::Position& square) {
    if (!game_engine.requestJump(square)) {
        return;
    }

    for (int i = 0; i < 160; ++i) {
        game_engine.advanceTime(kSyncFrameMs);
    }
}

}  // namespace

bool ClientGameSync::applyMessage(const std::string& json, engine::GameEngine& game_engine) {
    const std::optional<events::GameEvent> event = parseServerGameEvent(json);
    if (!event.has_value()) {
        return false;
    }

    if (const auto* move = std::get_if<realtime::CompletedMoveEvent>(&*event)) {
        fastForwardMove(game_engine, move->from, move->to);
        return true;
    }

    if (const auto* jump = std::get_if<realtime::JumpCaptureEvent>(&*event)) {
        fastForwardJump(game_engine, jump->jump_square);
        return true;
    }

    if (const auto* ended = std::get_if<events::GameEnded>(&*event)) {
        game_engine.applyRemoteGameEnded(ended->winner);
        return true;
    }

    return false;
}

}  // namespace network
