#include "message_router.h"

#include <optional>
#include <variant>

namespace network {

namespace {

std::optional<std::uint64_t> parseConnectionId(const ix::ConnectionState& connection_state) {
    try {
        return static_cast<std::uint64_t>(std::stoull(connection_state.getId()));
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

}  // namespace

MessageRouter::MessageRouter(session::SessionManager& session, CommandQueue& command_queue,
                             LoginHandler& login_handler, QueueHandler& queue_handler,
                             DisconnectHandler& disconnect_handler, ClientRegistry& clients)
    : session_(session),
      command_queue_(command_queue),
      login_handler_(login_handler),
      queue_handler_(queue_handler),
      disconnect_handler_(disconnect_handler),
      clients_(clients) {}

void MessageRouter::handleClientMessage(
    const std::shared_ptr<ix::ConnectionState>& connection_state, ix::WebSocket& web_socket,
    const ix::WebSocketMessagePtr& message) {
    const std::optional<std::uint64_t> connection_id = parseConnectionId(*connection_state);
    if (!connection_id.has_value()) {
        return;
    }

    if (message->type == ix::WebSocketMessageType::Open) {
        clients_.registerClient(*connection_id, &web_socket);
        return;
    }

    if (message->type == ix::WebSocketMessageType::Close) {
        disconnect_handler_.handleDisconnect(*connection_id);
        return;
    }

    if (message->type != ix::WebSocketMessageType::Message || message->binary) {
        return;
    }

    const std::optional<ClientCommand> command = parseClientMessage(message->str);
    if (!command.has_value()) {
        web_socket.sendText(serializeErrorMessage("Invalid message"));
        return;
    }

    if (std::holds_alternative<LoginCommand>(*command)) {
        login_handler_.handleLogin(*connection_id, web_socket, std::get<LoginCommand>(*command));
        return;
    }

    if (std::holds_alternative<JoinQueueCommand>(*command)) {
        queue_handler_.handleJoinQueue(*connection_id, web_socket);
        return;
    }

    if (std::holds_alternative<LeaveQueueCommand>(*command)) {
        queue_handler_.handleLeaveQueue(*connection_id, web_socket);
        return;
    }

    if (!session_.colorFor(*connection_id).has_value()) {
        web_socket.sendText(serializeErrorMessage("Not authenticated"));
        return;
    }

    command_queue_.push({*connection_id, *command});
}

}  // namespace network
