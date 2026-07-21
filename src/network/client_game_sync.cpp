#include "client_game_sync.h"

#include "../engine/game_engine.h"
#include "../model/board.h"
#include "ws_protocol.h"

namespace network {

namespace {

constexpr int kSyncFrameMs = 16;
constexpr int kMaxSyncAdvanceMs = 12000;

bool pieceAt(const model::Board& board, const model::Position& square, model::PieceType type,
             model::Color color) {
    if (!board.inBounds(square)) {
        return false;
    }

    const model::Piece& piece = board.cell(square);
    return !piece.isEmpty() && piece.type() == type && piece.color() == color;
}

void advanceUntilIdle(engine::GameEngine& game_engine) {
    int advanced_ms = 0;
    while (!game_engine.activeMotions().empty() && advanced_ms < kMaxSyncAdvanceMs) {
        game_engine.advanceTime(kSyncFrameMs);
        advanced_ms += kSyncFrameMs;
    }
}

// Begin animating a remote move locally the moment it is initiated on the host.
// requestMove starts a motion that the client's main loop interpolates over its full
// duration, so the piece slides just like a locally initiated move. If the square is
// already busy (e.g. the client's own optimistic move is animating) requestMove is a
// no-op, which harmlessly deduplicates.
void startMove(engine::GameEngine& game_engine, const model::Position& from,
               const model::Position& to) {
    game_engine.requestMove(from, to);
}

void startJump(engine::GameEngine& game_engine, const model::Position& square) {
    game_engine.requestJump(square);
}

// Completion events are now reconciliation only: the animation was already started by
// the corresponding move_started event. We only fast-forward as a fallback if the start
// was somehow missed, so an in-progress animation is left to finish naturally.
void syncCompletedMove(engine::GameEngine& game_engine, const model::Position& from,
                       const model::Position& to, model::PieceType piece_type,
                       model::Color piece_color) {
    if (pieceAt(game_engine.board(), to, piece_type, piece_color) &&
        !game_engine.isBusyAt(from)) {
        return;
    }

    // Animation started via move_started is still playing; let it complete on its own.
    if (game_engine.isBusyAt(from) || game_engine.isBusyAt(to)) {
        return;
    }

    // Fallback: the move_started event was missed, apply the move now to stay consistent.
    if (!game_engine.requestMove(from, to)) {
        return;
    }

    advanceUntilIdle(game_engine);
}

void syncJumpCapture(engine::GameEngine& game_engine, const model::Position& jump_square) {
    // Jump animation started via jump_started is still playing; let it finish naturally.
    if (game_engine.isBusyAt(jump_square)) {
        return;
    }

    // Fallback: the jump_started event was missed, apply it now to stay consistent.
    if (!game_engine.requestJump(jump_square)) {
        return;
    }

    advanceUntilIdle(game_engine);
}

}  // namespace

bool ClientGameSync::applyMessage(const std::string& json, engine::GameEngine& game_engine) {
    const std::optional<events::GameEvent> event = parseServerGameEvent(json);
    if (!event.has_value()) {
        return false;
    }

    if (const auto* started = std::get_if<realtime::MoveStartedEvent>(&*event)) {
        startMove(game_engine, started->from, started->to);
        return true;
    }

    if (const auto* started = std::get_if<realtime::JumpStartedEvent>(&*event)) {
        startJump(game_engine, started->square);
        return true;
    }

    if (const auto* move = std::get_if<realtime::CompletedMoveEvent>(&*event)) {
        syncCompletedMove(game_engine, move->from, move->to, move->piece_type, move->piece_color);
        return true;
    }

    if (const auto* jump = std::get_if<realtime::JumpCaptureEvent>(&*event)) {
        syncJumpCapture(game_engine, jump->jump_square);
        return true;
    }

    if (const auto* ended = std::get_if<events::GameEnded>(&*event)) {
        game_engine.applyRemoteGameEnded(ended->winner);
        return true;
    }

    return false;
}

}  // namespace network
