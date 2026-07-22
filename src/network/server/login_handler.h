#pragma once

#include <ixwebsocket/IXWebSocket.h>

#include <cstdint>

#include "../../online/disconnect_manager.h"
#include "../../online/session_manager.h"
#include "../ws_protocol.h"
#include "client_registry.h"

namespace network {

class LoginHandler {
public:
    LoginHandler(session::SessionManager& session, session::DisconnectManager& disconnect_manager,
                 ClientRegistry& clients);

    void handleLogin(std::uint64_t connection_id, ix::WebSocket& web_socket,
                     const LoginCommand& login);

private:
    session::SessionManager& session_;
    session::DisconnectManager& disconnect_manager_;
    ClientRegistry& clients_;
};

}  // namespace network
