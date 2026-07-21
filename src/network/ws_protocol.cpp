#include "ws_protocol.h"

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

#include "../engine/move_log.h"
#include "../io/algebraic.h"

namespace network {

namespace {

std::string escapeJsonString(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            default:
                escaped += ch;
                break;
        }
    }

    return escaped;
}

std::string quoteJsonString(const std::string& value) {
    return "\"" + escapeJsonString(value) + "\"";
}

const char* skipWhitespace(const char* cursor) {
    while (*cursor != '\0' && std::isspace(static_cast<unsigned char>(*cursor)) != 0) {
        ++cursor;
    }
    return cursor;
}

std::optional<std::string> readJsonStringValue(const char* cursor) {
    cursor = skipWhitespace(cursor);
    if (*cursor != '"') {
        return std::nullopt;
    }

    ++cursor;
    std::string value;
    while (*cursor != '\0') {
        if (*cursor == '"') {
            return value;
        }

        if (*cursor == '\\') {
            ++cursor;
            if (*cursor == '\0') {
                return std::nullopt;
            }
            value += *cursor;
            ++cursor;
            continue;
        }

        value += *cursor;
        ++cursor;
    }

    return std::nullopt;
}

std::optional<std::string> extractJsonString(const std::string& json, const std::string& key) {
    const std::string needle = quoteJsonString(key);
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return std::nullopt;
    }

    const char* cursor = json.c_str() + key_pos + needle.size();
    cursor = skipWhitespace(cursor);
    if (*cursor != ':') {
        return std::nullopt;
    }

    ++cursor;
    return readJsonStringValue(cursor);
}

std::optional<int> extractJsonInt(const std::string& json, const std::string& key) {
    const std::string needle = quoteJsonString(key);
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return std::nullopt;
    }

    const char* cursor = json.c_str() + key_pos + needle.size();
    cursor = skipWhitespace(cursor);
    if (*cursor != ':') {
        return std::nullopt;
    }

    ++cursor;
    cursor = skipWhitespace(cursor);
    char* end = nullptr;
    const long value = std::strtol(cursor, &end, 10);
    if (cursor == end) {
        return std::nullopt;
    }
    return static_cast<int>(value);
}

bool parsePieceLabel(const std::string& label, model::PieceType& type, model::Color& color) {
    if (label.size() != 2) {
        return false;
    }

    if (label[0] == 'w') {
        color = model::Color::White;
    } else if (label[0] == 'b') {
        color = model::Color::Black;
    } else {
        return false;
    }

    switch (label[1]) {
        case 'K':
            type = model::PieceType::King;
            return true;
        case 'Q':
            type = model::PieceType::Queen;
            return true;
        case 'R':
            type = model::PieceType::Rook;
            return true;
        case 'B':
            type = model::PieceType::Bishop;
            return true;
        case 'N':
            type = model::PieceType::Knight;
            return true;
        case 'P':
            type = model::PieceType::Pawn;
            return true;
        default:
            return false;
    }
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
    const std::string winner_label = event.winner == model::Color::White ? "white" : "black";
    return "{\"type\":\"game_ended\",\"winner\":" + quoteJsonString(winner_label) + '}';
}

}  // namespace

std::optional<ClientCommand> parseClientMessage(const std::string& json) {
    const std::optional<std::string> type = extractJsonString(json, "type");
    if (!type.has_value()) {
        return std::nullopt;
    }

    if (*type == "login") {
        const std::optional<std::string> username = extractJsonString(json, "username");
        if (!username.has_value() || username->empty()) {
            return std::nullopt;
        }
        return LoginCommand{*username};
    }

    if (*type == "move") {
        const std::optional<std::string> from = extractJsonString(json, "from");
        const std::optional<std::string> to = extractJsonString(json, "to");
        if (!from.has_value() || !to.has_value() || from->empty() || to->empty()) {
            return std::nullopt;
        }
        return MoveCommand{*from, *to};
    }

    if (*type == "jump") {
        const std::optional<std::string> square = extractJsonString(json, "square");
        if (!square.has_value() || square->empty()) {
            return std::nullopt;
        }
        return JumpCommand{*square};
    }

    if (*type == "join_queue") {
        return JoinQueueCommand{};
    }

    if (*type == "leave_queue") {
        return LeaveQueueCommand{};
    }

    return std::nullopt;
}

ServerMessageType parseServerMessageType(const std::string& json) {
    const std::optional<std::string> type = extractJsonString(json, "type");
    if (!type.has_value()) {
        return ServerMessageType::Unknown;
    }

    if (*type == "login_ok") {
        return ServerMessageType::LoginOk;
    }
    if (*type == "error") {
        return ServerMessageType::Error;
    }
    if (*type == "queue_joined") {
        return ServerMessageType::QueueJoined;
    }
    if (*type == "queue_left") {
        return ServerMessageType::QueueLeft;
    }
    if (*type == "match_found") {
        return ServerMessageType::MatchFound;
    }
    if (*type == "game_started") {
        return ServerMessageType::GameStarted;
    }
    if (*type == "move_completed") {
        return ServerMessageType::MoveCompleted;
    }
    if (*type == "jump_capture") {
        return ServerMessageType::JumpCapture;
    }
    if (*type == "game_ended") {
        return ServerMessageType::GameEnded;
    }

    return ServerMessageType::Unknown;
}

model::Color colorFromLabel(const std::string& label) {
    return label == "black" ? model::Color::Black : model::Color::White;
}

std::string colorLabel(model::Color color) {
    return color == model::Color::White ? "white" : "black";
}

std::optional<MatchFoundMessage> parseMatchFoundMessage(const std::string& json) {
    if (parseServerMessageType(json) != ServerMessageType::MatchFound) {
        return std::nullopt;
    }

    const std::optional<std::string> color = extractJsonString(json, "color");
    const std::optional<std::string> opponent = extractJsonString(json, "opponent");
    const std::optional<int> port = extractJsonInt(json, "port");
    if (!color.has_value() || !opponent.has_value() || !port.has_value()) {
        return std::nullopt;
    }

    return MatchFoundMessage{colorFromLabel(*color), *opponent, *port};
}

std::optional<GameStartedMessage> parseGameStartedMessage(const std::string& json) {
    if (parseServerMessageType(json) != ServerMessageType::GameStarted) {
        return std::nullopt;
    }

    const std::optional<std::string> white = extractJsonString(json, "white");
    const std::optional<std::string> black = extractJsonString(json, "black");
    const std::optional<int> port = extractJsonInt(json, "port");
    if (!white.has_value() || !black.has_value() || !port.has_value()) {
        return std::nullopt;
    }

    return GameStartedMessage{*white, *black, *port};
}

std::optional<events::GameEvent> parseServerGameEvent(const std::string& json) {
    const ServerMessageType type = parseServerMessageType(json);
    if (type == ServerMessageType::GameStarted) {
        const std::optional<GameStartedMessage> started = parseGameStartedMessage(json);
        if (!started.has_value()) {
            return std::nullopt;
        }
        return events::GameStarted{started->white_user, started->black_user, started->port};
    }

    if (type == ServerMessageType::MoveCompleted) {
        realtime::CompletedMoveEvent event;
        const std::optional<int> timestamp = extractJsonInt(json, "timestamp_ms");
        const std::optional<std::string> piece = extractJsonString(json, "piece");
        const std::optional<std::string> from = extractJsonString(json, "from");
        const std::optional<std::string> to = extractJsonString(json, "to");
        if (!timestamp.has_value() || !piece.has_value() || !from.has_value() || !to.has_value()) {
            return std::nullopt;
        }

        if (!parsePieceLabel(*piece, event.piece_type, event.piece_color)) {
            return std::nullopt;
        }

        event.timestamp_ms = *timestamp;
        event.from = io::algebraicToPosition(*from);
        event.to = io::algebraicToPosition(*to);
        if (event.from.row < 0 || event.to.row < 0) {
            return std::nullopt;
        }

        const std::optional<std::string> captured = extractJsonString(json, "captured");
        if (captured.has_value() &&
            parsePieceLabel(*captured, event.captured_type, event.captured_color)) {
            // captured fields set by parsePieceLabel
        } else {
            event.captured_type = model::PieceType::Empty;
        }

        return event;
    }

    if (type == ServerMessageType::JumpCapture) {
        realtime::JumpCaptureEvent event;
        const std::optional<int> timestamp = extractJsonInt(json, "timestamp_ms");
        const std::optional<std::string> jumper = extractJsonString(json, "jumper");
        const std::optional<std::string> victim = extractJsonString(json, "victim");
        const std::optional<std::string> jump_square = extractJsonString(json, "jump_square");
        const std::optional<std::string> victim_from = extractJsonString(json, "victim_from");
        const std::optional<std::string> victim_to = extractJsonString(json, "victim_to");
        if (!timestamp.has_value() || !jumper.has_value() || !victim.has_value() ||
            !jump_square.has_value() || !victim_from.has_value() || !victim_to.has_value()) {
            return std::nullopt;
        }

        if (!parsePieceLabel(*jumper, event.jumper_type, event.jumper_color) ||
            !parsePieceLabel(*victim, event.victim_type, event.victim_color)) {
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
        const std::optional<std::string> winner = extractJsonString(json, "winner");
        if (!winner.has_value()) {
            return std::nullopt;
        }
        return events::GameEnded{colorFromLabel(*winner)};
    }

    return std::nullopt;
}

std::string serializeServerEvent(const events::GameEvent& event) {
    if (std::holds_alternative<events::GameStarted>(event)) {
        const auto& started = std::get<events::GameStarted>(event);
        std::ostringstream out;
        out << "{\"type\":\"game_started\""
            << ",\"white\":" << quoteJsonString(started.white_user) << ",\"black\":"
            << quoteJsonString(started.black_user) << ",\"port\":" << started.port << '}';
        return out.str();
    }

    if (std::holds_alternative<realtime::CompletedMoveEvent>(event)) {
        return serializeCompletedMove(std::get<realtime::CompletedMoveEvent>(event));
    }

    if (std::holds_alternative<realtime::JumpCaptureEvent>(event)) {
        return serializeJumpCapture(std::get<realtime::JumpCaptureEvent>(event));
    }

    return serializeGameEnded(std::get<events::GameEnded>(event));
}

std::string serializeErrorMessage(const std::string& message) {
    return "{\"type\":\"error\",\"message\":" + quoteJsonString(message) + '}';
}

std::string serializeLoginOk(const std::string& color) {
    return "{\"type\":\"login_ok\",\"color\":" + quoteJsonString(color) + '}';
}

std::string serializeLoginCommand(const std::string& username) {
    return "{\"type\":\"login\",\"username\":" + quoteJsonString(username) + '}';
}

std::string serializeMoveCommand(const std::string& from, const std::string& to) {
    return "{\"type\":\"move\",\"from\":" + quoteJsonString(from) + ",\"to\":" +
           quoteJsonString(to) + '}';
}

std::string serializeJumpCommand(const std::string& square) {
    return "{\"type\":\"jump\",\"square\":" + quoteJsonString(square) + '}';
}

std::string serializeJoinQueueCommand() { return R"({"type":"join_queue"})"; }

std::string serializeLeaveQueueCommand() { return R"({"type":"leave_queue"})"; }

std::string serializeQueueJoined() { return R"({"type":"queue_joined"})"; }

std::string serializeQueueLeft() { return R"({"type":"queue_left"})"; }

std::string serializeMatchFound(const std::string& color, const std::string& opponent, int port) {
    std::ostringstream out;
    out << "{\"type\":\"match_found\""
        << ",\"color\":" << quoteJsonString(color) << ",\"opponent\":"
        << quoteJsonString(opponent) << ",\"port\":" << port << '}';
    return out.str();
}

}  // namespace network
