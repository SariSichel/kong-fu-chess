#include "../ws_protocol.h"

#include "json_util.h"

namespace network {
namespace {

std::optional<LoginCommand> parseLoginCommand(const std::string& json) {
    const std::optional<std::string> username = protocol::extractJsonString(json, "username");
    if (!username.has_value() || username->empty()) {
        return std::nullopt;
    }
    return LoginCommand{*username};
}

std::optional<MoveCommand> parseMoveCommand(const std::string& json) {
    const std::optional<std::string> from = protocol::extractJsonString(json, "from");
    const std::optional<std::string> to = protocol::extractJsonString(json, "to");
    if (!from.has_value() || !to.has_value() || from->empty() || to->empty()) {
        return std::nullopt;
    }
    return MoveCommand{*from, *to};
}

std::optional<JumpCommand> parseJumpCommand(const std::string& json) {
    const std::optional<std::string> square = protocol::extractJsonString(json, "square");
    if (!square.has_value() || square->empty()) {
        return std::nullopt;
    }
    return JumpCommand{*square};
}

}  // namespace

std::optional<ClientCommand> parseClientMessage(const std::string& json) {
    const std::optional<std::string> type = protocol::extractJsonString(json, "type");
    if (!type.has_value()) {
        return std::nullopt;
    }

    if (*type == "login") {
        if (const std::optional<LoginCommand> command = parseLoginCommand(json)) {
            return *command;
        }
        return std::nullopt;
    }

    if (*type == "move") {
        if (const std::optional<MoveCommand> command = parseMoveCommand(json)) {
            return *command;
        }
        return std::nullopt;
    }

    if (*type == "jump") {
        if (const std::optional<JumpCommand> command = parseJumpCommand(json)) {
            return *command;
        }
        return std::nullopt;
    }

    if (*type == "join_queue") {
        return JoinQueueCommand{};
    }

    if (*type == "leave_queue") {
        return LeaveQueueCommand{};
    }

    return std::nullopt;
}

std::string serializeLoginCommand(const std::string& username) {
    return "{\"type\":\"login\",\"username\":" + protocol::quoteJsonString(username) + '}';
}

std::string serializeMoveCommand(const std::string& from, const std::string& to) {
    return "{\"type\":\"move\",\"from\":" + protocol::quoteJsonString(from) + ",\"to\":" +
           protocol::quoteJsonString(to) + '}';
}

std::string serializeJumpCommand(const std::string& square) {
    return "{\"type\":\"jump\",\"square\":" + protocol::quoteJsonString(square) + '}';
}

std::string serializeJoinQueueCommand() { return R"({"type":"join_queue"})"; }

std::string serializeLeaveQueueCommand() { return R"({"type":"leave_queue"})"; }

}  // namespace network
