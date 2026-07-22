#include "login_handler.h"

namespace network {

namespace {

std::string authErrorMessage(session::AuthResult result) {
    switch (result) {
        case session::AuthResult::UnknownUser:
            return "Unknown username";
        case session::AuthResult::SlotFull:
            return "Session full";
        case session::AuthResult::AlreadyConnected:
            return "Username already connected";
        case session::AuthResult::Accepted:
        default:
            return "Login accepted";
    }
}

}  // namespace

LoginHandler::LoginHandler(session::SessionManager& session,
                           session::DisconnectManager& disconnect_manager, ClientRegistry& clients)
    : session_(session), disconnect_manager_(disconnect_manager), clients_(clients) {}

void LoginHandler::handleLogin(std::uint64_t connection_id, ix::WebSocket& web_socket,
                               const LoginCommand& login) {
    if (disconnect_manager_.isInGrace(login.username)) {
        const session::AuthResult reconnect_result =
            session_.reconnect(connection_id, login.username);
        if (reconnect_result != session::AuthResult::Accepted) {
            web_socket.sendText(serializeErrorMessage(authErrorMessage(reconnect_result)));
            return;
        }

        disconnect_manager_.onReconnect(login.username);
        clients_.broadcast(serializePlayerReconnected(login.username));

        const std::optional<model::Color> color = session_.colorFor(connection_id);
        if (!color.has_value()) {
            web_socket.sendText(serializeErrorMessage("Reconnect failed"));
            return;
        }

        web_socket.sendText(serializeLoginOk(colorLabel(*color)));
        web_socket.sendText(serializeServerEvent(session_.rosterSnapshot()));
        return;
    }

    const session::AuthResult result = session_.authenticate(connection_id, login.username);
    if (result != session::AuthResult::Accepted) {
        web_socket.sendText(serializeErrorMessage(authErrorMessage(result)));
        return;
    }

    const std::optional<model::Color> color = session_.colorFor(connection_id);
    if (!color.has_value()) {
        web_socket.sendText(serializeLoginOk("none"));
        return;
    }

    web_socket.sendText(serializeLoginOk(colorLabel(*color)));
}

}  // namespace network
