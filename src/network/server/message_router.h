#pragma once

#include <ixwebsocket/IXConnectionState.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketMessage.h>

#include <memory>

#include "../../online/session_manager.h"
#include "../command_queue.h"
#include "../ws_protocol.h"
#include "client_registry.h"
#include "disconnect_handler.h"
#include "login_handler.h"
#include "queue_handler.h"

namespace network {

class MessageRouter {
public:
    MessageRouter(session::SessionManager& session, CommandQueue& command_queue,
                  LoginHandler& login_handler, QueueHandler& queue_handler,
                  DisconnectHandler& disconnect_handler, ClientRegistry& clients);

    void handleClientMessage(const std::shared_ptr<ix::ConnectionState>& connection_state,
                             ix::WebSocket& web_socket, const ix::WebSocketMessagePtr& message);

private:
    session::SessionManager& session_;
    CommandQueue& command_queue_;
    LoginHandler& login_handler_;
    QueueHandler& queue_handler_;
    DisconnectHandler& disconnect_handler_;
    ClientRegistry& clients_;
};

}  // namespace network
