#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include "../events/game_event.h"

namespace network {

struct LoginCommand {
    std::string username;
};

struct MoveCommand {
    std::string from;
    std::string to;
};

struct JumpCommand {
    std::string square;
};

struct JoinQueueCommand {};

struct LeaveQueueCommand {};

using ClientCommand =
    std::variant<LoginCommand, MoveCommand, JumpCommand, JoinQueueCommand, LeaveQueueCommand>;

enum class ClientCommandType {
    Login = 0,
    Move = 1,
    Jump = 2,
    JoinQueue = 3,
    LeaveQueue = 4,
};

enum class ServerMessageType {
    Unknown,
    LoginOk,
    Error,
    QueueJoined,
    QueueLeft,
};

inline ClientCommandType clientCommandType(const ClientCommand& command) {
    return static_cast<ClientCommandType>(command.index());
}

std::optional<ClientCommand> parseClientMessage(const std::string& json);
ServerMessageType parseServerMessageType(const std::string& json);
std::string serializeLoginCommand(const std::string& username);
std::string serializeMoveCommand(const std::string& from, const std::string& to);
std::string serializeJumpCommand(const std::string& square);
std::string serializeJoinQueueCommand();
std::string serializeLeaveQueueCommand();
std::string serializeServerEvent(const events::GameEvent& event);
std::string serializeErrorMessage(const std::string& message);
std::string serializeLoginOk(const std::string& color);
std::string serializeQueueJoined();
std::string serializeQueueLeft();

}  // namespace network
