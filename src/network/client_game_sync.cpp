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

void syncCompletedMove(engine::GameEngine& game_engine, const model::Position& from,
                       const model::Position& to, model::PieceType piece_type,
                       model::Color piece_color) {
    if (pieceAt(game_engine.board(), to, piece_type, piece_color) &&
        !game_engine.isBusyAt(from)) {
        return;
    }

    if (game_engine.isBusyAt(from) || game_engine.isBusyAt(to)) {
        advanceUntilIdle(game_engine);
        if (pieceAt(game_engine.board(), to, piece_type, piece_color)) {
            return;
        }
    }

    if (!game_engine.requestMove(from, to)) {
        return;
    }

    advanceUntilIdle(game_engine);
}

void syncJumpCapture(engine::GameEngine& game_engine, const model::Position& jump_square) {
    if (game_engine.isBusyAt(jump_square)) {
        advanceUntilIdle(game_engine);
        return;
    }

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
