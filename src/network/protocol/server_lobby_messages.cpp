#include "../ws_protocol.h"

#include "json_util.h"

#include <sstream>

namespace network {

ServerMessageType parseServerMessageType(const std::string& json) {
    const std::optional<std::string> type = protocol::extractJsonString(json, "type");
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
    if (*type == "move_started") {
        return ServerMessageType::MoveStarted;
    }
    if (*type == "move_completed") {
        return ServerMessageType::MoveCompleted;
    }
    if (*type == "jump_started") {
        return ServerMessageType::JumpStarted;
    }
    if (*type == "jump_capture") {
        return ServerMessageType::JumpCapture;
    }
    if (*type == "game_ended") {
        return ServerMessageType::GameEnded;
    }
    if (*type == "queue_timeout") {
        return ServerMessageType::QueueTimeout;
    }
    if (*type == "player_disconnected") {
        return ServerMessageType::PlayerDisconnected;
    }
    if (*type == "player_reconnected") {
        return ServerMessageType::PlayerReconnected;
    }

    return ServerMessageType::Unknown;
}

std::string serializeErrorMessage(const std::string& message) {
    return "{\"type\":\"error\",\"message\":" + protocol::quoteJsonString(message) + '}';
}

std::string serializeLoginOk(const std::string& color) {
    return "{\"type\":\"login_ok\",\"color\":" + protocol::quoteJsonString(color) + '}';
}

std::string serializeQueueJoined() { return R"({"type":"queue_joined"})"; }

std::string serializeQueueLeft() { return R"({"type":"queue_left"})"; }

std::string serializeQueueTimeout(const std::string& message) {
    return "{\"type\":\"queue_timeout\",\"message\":" + protocol::quoteJsonString(message) + '}';
}

std::string serializeMatchFound(const std::string& color, const std::string& opponent, int port) {
    std::ostringstream out;
    out << "{\"type\":\"match_found\""
        << ",\"color\":" << protocol::quoteJsonString(color) << ",\"opponent\":"
        << protocol::quoteJsonString(opponent) << ",\"port\":" << port << '}';
    return out.str();
}

std::string serializePlayerDisconnected(const std::string& username, const std::string& color,
                                        int grace_seconds) {
    std::ostringstream out;
    out << "{\"type\":\"player_disconnected\""
        << ",\"username\":" << protocol::quoteJsonString(username) << ",\"color\":"
        << protocol::quoteJsonString(color) << ",\"grace_seconds\":" << grace_seconds << '}';
    return out.str();
}

std::string serializePlayerReconnected(const std::string& username) {
    return "{\"type\":\"player_reconnected\",\"username\":" + protocol::quoteJsonString(username) +
           '}';
}

std::optional<MatchFoundMessage> parseMatchFoundMessage(const std::string& json) {
    if (parseServerMessageType(json) != ServerMessageType::MatchFound) {
        return std::nullopt;
    }

    const std::optional<std::string> color = protocol::extractJsonString(json, "color");
    const std::optional<std::string> opponent = protocol::extractJsonString(json, "opponent");
    const std::optional<int> port = protocol::extractJsonInt(json, "port");
    if (!color.has_value() || !opponent.has_value() || !port.has_value()) {
        return std::nullopt;
    }

    return MatchFoundMessage{colorFromLabel(*color), *opponent, *port};
}

std::optional<model::Color> parseLoginOkColor(const std::string& json) {
    if (parseServerMessageType(json) != ServerMessageType::LoginOk) {
        return std::nullopt;
    }

    const std::optional<std::string> color = protocol::extractJsonString(json, "color");
    if (!color.has_value() || *color == "none") {
        return std::nullopt;
    }

    return colorFromLabel(*color);
}

std::optional<GameStartedMessage> parseGameStartedMessage(const std::string& json) {
    if (parseServerMessageType(json) != ServerMessageType::GameStarted) {
        return std::nullopt;
    }

    const std::optional<std::string> white = protocol::extractJsonString(json, "white");
    const std::optional<std::string> black = protocol::extractJsonString(json, "black");
    const std::optional<int> port = protocol::extractJsonInt(json, "port");
    if (!white.has_value() || !black.has_value() || !port.has_value()) {
        return std::nullopt;
    }

    return GameStartedMessage{*white, *black, *port};
}

std::optional<QueueTimeoutMessage> parseQueueTimeoutMessage(const std::string& json) {
    if (parseServerMessageType(json) != ServerMessageType::QueueTimeout) {
        return std::nullopt;
    }

    const std::optional<std::string> message = protocol::extractJsonString(json, "message");
    if (!message.has_value()) {
        return std::nullopt;
    }

    return QueueTimeoutMessage{*message};
}

std::optional<PlayerDisconnectedMessage> parsePlayerDisconnectedMessage(const std::string& json) {
    if (parseServerMessageType(json) != ServerMessageType::PlayerDisconnected) {
        return std::nullopt;
    }

    const std::optional<std::string> username = protocol::extractJsonString(json, "username");
    const std::optional<std::string> color = protocol::extractJsonString(json, "color");
    const std::optional<int> grace_seconds = protocol::extractJsonInt(json, "grace_seconds");
    if (!username.has_value() || !color.has_value() || !grace_seconds.has_value()) {
        return std::nullopt;
    }

    return PlayerDisconnectedMessage{*username, colorFromLabel(*color), *grace_seconds};
}

std::optional<PlayerReconnectedMessage> parsePlayerReconnectedMessage(const std::string& json) {
    if (parseServerMessageType(json) != ServerMessageType::PlayerReconnected) {
        return std::nullopt;
    }

    const std::optional<std::string> username = protocol::extractJsonString(json, "username");
    if (!username.has_value()) {
        return std::nullopt;
    }

    return PlayerReconnectedMessage{*username};
}

}  // namespace network
