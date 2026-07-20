#include "ws_protocol.h"

#include <cctype>
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

}  // namespace network
