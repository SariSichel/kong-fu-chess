#include "../ws_protocol.h"

#include "../../engine/move_log.h"
#include "../../events/game_event.h"
#include "../../io/algebraic.h"

#include "json_util.h"
#include "piece_codec.h"

#include <sstream>

namespace network {
namespace protocol {

std::string serializeGameStarted(const events::GameStarted& started) {
    std::ostringstream out;
    out << "{\"type\":\"game_started\""
        << ",\"white\":" << quoteJsonString(started.white_user) << ",\"black\":"
        << quoteJsonString(started.black_user) << ",\"port\":" << started.port << '}';
    return out.str();
}

std::string serializeMoveStarted(const realtime::MoveStartedEvent& event) {
    std::ostringstream out;
    out << "{\"type\":\"move_started\""
        << ",\"timestamp_ms\":" << event.timestamp_ms << ",\"piece\":"
        << quoteJsonString(engine::MoveLog::pieceLabel(event.piece_type, event.piece_color))
        << ",\"from\":" << quoteJsonString(io::positionToAlgebraic(event.from))
        << ",\"to\":" << quoteJsonString(io::positionToAlgebraic(event.to)) << '}';
    return out.str();
}

std::string serializeJumpStarted(const realtime::JumpStartedEvent& event) {
    std::ostringstream out;
    out << "{\"type\":\"jump_started\""
        << ",\"timestamp_ms\":" << event.timestamp_ms << ",\"piece\":"
        << quoteJsonString(engine::MoveLog::pieceLabel(event.piece_type, event.piece_color))
        << ",\"square\":" << quoteJsonString(io::positionToAlgebraic(event.square)) << '}';
    return out.str();
}

std::string serializeCompletedMove(const realtime::CompletedMoveEvent& event) {
    std::ostringstream out;
    out << "{\"type\":\"move_completed\""
        << ",\"timestamp_ms\":" << event.timestamp_ms << ",\"piece\":"
        << quoteJsonString(engine::MoveLog::pieceLabel(event.piece_type, event.piece_color))
        << ",\"from\":" << quoteJsonString(io::positionToAlgebraic(event.from)) << ",\"to\":"
        << quoteJsonString(io::positionToAlgebraic(event.to));

    if (event.captured_type != model::PieceType::Empty) {
        out << ",\"captured\":"
            << quoteJsonString(
                   engine::MoveLog::pieceLabel(event.captured_type, event.captured_color));
    }

    out << '}';
    return out.str();
}

std::string serializeJumpCapture(const realtime::JumpCaptureEvent& event) {
    std::ostringstream out;
    out << "{\"type\":\"jump_capture\""
        << ",\"timestamp_ms\":" << event.timestamp_ms << ",\"jumper\":"
        << quoteJsonString(engine::MoveLog::pieceLabel(event.jumper_type, event.jumper_color))
        << ",\"victim\":"
        << quoteJsonString(engine::MoveLog::pieceLabel(event.victim_type, event.victim_color))
        << ",\"jump_square\":"
        << quoteJsonString(io::positionToAlgebraic(event.jump_square)) << ",\"victim_from\":"
        << quoteJsonString(io::positionToAlgebraic(event.victim_from)) << ",\"victim_to\":"
        << quoteJsonString(io::positionToAlgebraic(event.victim_to)) << '}';
    return out.str();
}

std::string serializeGameEnded(const events::GameEnded& event) {
    const std::string winner_label = network::colorLabel(event.winner);
    return "{\"type\":\"game_ended\",\"winner\":" + quoteJsonString(winner_label) + '}';
}

}  // namespace protocol

std::optional<events::GameEvent> parseServerGameEvent(const std::string& json) {
    const ServerMessageType type = parseServerMessageType(json);
    if (type == ServerMessageType::GameStarted) {
        const std::optional<GameStartedMessage> started = parseGameStartedMessage(json);
        if (!started.has_value()) {
            return std::nullopt;
        }
        return events::GameStarted{started->white_user, started->black_user, started->port};
    }

    if (type == ServerMessageType::MoveStarted) {
        realtime::MoveStartedEvent event;
        const std::optional<int> timestamp = protocol::extractJsonInt(json, "timestamp_ms");
        const std::optional<std::string> piece = protocol::extractJsonString(json, "piece");
        const std::optional<std::string> from = protocol::extractJsonString(json, "from");
        const std::optional<std::string> to = protocol::extractJsonString(json, "to");
        if (!timestamp.has_value() || !piece.has_value() || !from.has_value() || !to.has_value()) {
            return std::nullopt;
        }

        if (!protocol::parsePieceLabel(*piece, event.piece_type, event.piece_color)) {
            return std::nullopt;
        }

        event.timestamp_ms = *timestamp;
        event.from = io::algebraicToPosition(*from);
        event.to = io::algebraicToPosition(*to);
        if (event.from.row < 0 || event.to.row < 0) {
            return std::nullopt;
        }

        return event;
    }

    if (type == ServerMessageType::JumpStarted) {
        realtime::JumpStartedEvent event;
        const std::optional<int> timestamp = protocol::extractJsonInt(json, "timestamp_ms");
        const std::optional<std::string> piece = protocol::extractJsonString(json, "piece");
        const std::optional<std::string> square = protocol::extractJsonString(json, "square");
        if (!timestamp.has_value() || !piece.has_value() || !square.has_value()) {
            return std::nullopt;
        }

        if (!protocol::parsePieceLabel(*piece, event.piece_type, event.piece_color)) {
            return std::nullopt;
        }

        event.timestamp_ms = *timestamp;
        event.square = io::algebraicToPosition(*square);
        if (event.square.row < 0) {
            return std::nullopt;
        }

        return event;
    }

    if (type == ServerMessageType::MoveCompleted) {
        realtime::CompletedMoveEvent event;
        const std::optional<int> timestamp = protocol::extractJsonInt(json, "timestamp_ms");
        const std::optional<std::string> piece = protocol::extractJsonString(json, "piece");
        const std::optional<std::string> from = protocol::extractJsonString(json, "from");
        const std::optional<std::string> to = protocol::extractJsonString(json, "to");
        if (!timestamp.has_value() || !piece.has_value() || !from.has_value() || !to.has_value()) {
            return std::nullopt;
        }

        if (!protocol::parsePieceLabel(*piece, event.piece_type, event.piece_color)) {
            return std::nullopt;
        }

        event.timestamp_ms = *timestamp;
        event.from = io::algebraicToPosition(*from);
        event.to = io::algebraicToPosition(*to);
        if (event.from.row < 0 || event.to.row < 0) {
            return std::nullopt;
        }

        const std::optional<std::string> captured = protocol::extractJsonString(json, "captured");
        if (captured.has_value() &&
            protocol::parsePieceLabel(*captured, event.captured_type, event.captured_color)) {
            // captured fields set by parsePieceLabel
        } else {
            event.captured_type = model::PieceType::Empty;
        }

        return event;
    }

    if (type == ServerMessageType::JumpCapture) {
        realtime::JumpCaptureEvent event;
        const std::optional<int> timestamp = protocol::extractJsonInt(json, "timestamp_ms");
        const std::optional<std::string> jumper = protocol::extractJsonString(json, "jumper");
        const std::optional<std::string> victim = protocol::extractJsonString(json, "victim");
        const std::optional<std::string> jump_square = protocol::extractJsonString(json, "jump_square");
        const std::optional<std::string> victim_from = protocol::extractJsonString(json, "victim_from");
        const std::optional<std::string> victim_to = protocol::extractJsonString(json, "victim_to");
        if (!timestamp.has_value() || !jumper.has_value() || !victim.has_value() ||
            !jump_square.has_value() || !victim_from.has_value() || !victim_to.has_value()) {
            return std::nullopt;
        }

        if (!protocol::parsePieceLabel(*jumper, event.jumper_type, event.jumper_color) ||
            !protocol::parsePieceLabel(*victim, event.victim_type, event.victim_color)) {
            return std::nullopt;
        }

        event.timestamp_ms = *timestamp;
        event.jump_square = io::algebraicToPosition(*jump_square);
        event.victim_from = io::algebraicToPosition(*victim_from);
        event.victim_to = io::algebraicToPosition(*victim_to);
        if (event.jump_square.row < 0 || event.victim_from.row < 0 || event.victim_to.row < 0) {
            return std::nullopt;
        }

        return event;
    }

    if (type == ServerMessageType::GameEnded) {
        const std::optional<std::string> winner = protocol::extractJsonString(json, "winner");
        if (!winner.has_value()) {
            return std::nullopt;
        }
        return events::GameEnded{colorFromLabel(*winner)};
    }

    return std::nullopt;
}

std::string serializeServerEvent(const events::GameEvent& event) {
    if (std::holds_alternative<events::GameStarted>(event)) {
        return protocol::serializeGameStarted(std::get<events::GameStarted>(event));
    }

    if (std::holds_alternative<realtime::MoveStartedEvent>(event)) {
        return protocol::serializeMoveStarted(std::get<realtime::MoveStartedEvent>(event));
    }

    if (std::holds_alternative<realtime::CompletedMoveEvent>(event)) {
        return protocol::serializeCompletedMove(std::get<realtime::CompletedMoveEvent>(event));
    }

    if (std::holds_alternative<realtime::JumpStartedEvent>(event)) {
        return protocol::serializeJumpStarted(std::get<realtime::JumpStartedEvent>(event));
    }

    if (std::holds_alternative<realtime::JumpCaptureEvent>(event)) {
        return protocol::serializeJumpCapture(std::get<realtime::JumpCaptureEvent>(event));
    }

    return protocol::serializeGameEnded(std::get<events::GameEnded>(event));
}

}  // namespace network
