#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include "../events/game_event.h"
#include "../model/piece.h"

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
    MatchFound,
    GameStarted,
    MoveCompleted,
    JumpCapture,
    GameEnded,
    QueueTimeout,
    PlayerDisconnected,
    PlayerReconnected,
};

struct QueueTimeoutMessage {
    std::string message;
};

struct PlayerDisconnectedMessage {
    std::string username;
    model::Color color = model::Color::White;
    int grace_seconds = 0;
};

struct PlayerReconnectedMessage {
    std::string username;
};

struct MatchFoundMessage {
    model::Color color = model::Color::White;
    std::string opponent;
    int port = 0;
};

struct GameStartedMessage {
    std::string white_user;
    std::string black_user;
    int port = 0;
};

inline ClientCommandType clientCommandType(const ClientCommand& command) {
    return static_cast<ClientCommandType>(command.index());
}

std::optional<ClientCommand> parseClientMessage(const std::string& json);
ServerMessageType parseServerMessageType(const std::string& json);
std::optional<MatchFoundMessage> parseMatchFoundMessage(const std::string& json);
std::optional<GameStartedMessage> parseGameStartedMessage(const std::string& json);
std::optional<model::Color> parseLoginOkColor(const std::string& json);
std::optional<QueueTimeoutMessage> parseQueueTimeoutMessage(const std::string& json);
std::optional<PlayerDisconnectedMessage> parsePlayerDisconnectedMessage(const std::string& json);
std::optional<PlayerReconnectedMessage> parsePlayerReconnectedMessage(const std::string& json);
std::optional<events::GameEvent> parseServerGameEvent(const std::string& json);

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
std::string serializeMatchFound(const std::string& color, const std::string& opponent, int port);
std::string serializeQueueTimeout(const std::string& message);
std::string serializePlayerDisconnected(const std::string& username, const std::string& color,
                                        int grace_seconds);
std::string serializePlayerReconnected(const std::string& username);

model::Color colorFromLabel(const std::string& label);
std::string colorLabel(model::Color color);

}  // namespace network
