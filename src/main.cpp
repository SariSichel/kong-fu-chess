//git link:
//https://github.com/SariSichel/kong-fu-chess/tree/main

#include "db/user_store.h"
#include "events/event_bus.h"
#include "network/command_queue.h"
#include "network/local_ws_client.h"
#include "network/ws_server.h"
#include "session/session_manager.h"
#include "session/shell_login.h"
#include "session/shell_lobby.h"

#include "constants.h"

#include <iostream>
#include <optional>
#include <sstream>

namespace {

constexpr const char* kPendingOpponent = "__pending__";

}  // namespace

int main() {
    const session::PlayerCredentials credentials =
        session::ShellLogin::readPlayerCredentials(std::cin, std::cout);
    if (credentials.username.empty()) {
        std::cerr << "Login cancelled.\n";
        return 1;
    }

    db::UserStore store("kong_fu_chess.db");
    if (!store.isOpen()) {
        std::cerr << "Failed to open user database.\n";
        return 1;
    }

    const auto authenticateOrRegister = [&store](const std::string& username,
                                                  const std::string& password) {
        if (store.find(username).has_value()) {
            return store.verifyPassword(username, password);
        }
        return store.create(username, password);
    };

    if (!authenticateOrRegister(credentials.username, credentials.password)) {
        std::cerr << "Authentication failed for '" << credentials.username << "'.\n";
        return 1;
    }

    const std::optional<db::UserRecord> user_record = store.find(credentials.username);
    if (!user_record.has_value()) {
        std::cerr << "Failed to load user record.\n";
        return 1;
    }

    events::EventBus bus;
    session::SessionManager session(credentials.username, kPendingOpponent,
                                    NetworkConfig::kDefaultPort);

    network::CommandQueue command_queue;
    network::WsServer ws_server(session, bus, command_queue, NetworkConfig::kDefaultPort);

    if (!ws_server.start()) {
        std::cerr << "Failed to start WebSocket server on port " << ws_server.port() << ".\n";
        return 1;
    }

    std::ostringstream client_url;
    client_url << "ws://127.0.0.1:" << ws_server.port();
    network::LocalWsClient client(client_url.str(), credentials.username);

    session::ShellLobby lobby(credentials.username, user_record->elo);
    client.setMessageHandler([&lobby](const std::string& json) { lobby.handleServerMessage(json); });

    if (!client.connectAndLogin()) {
        std::cerr << "Failed to connect to local WebSocket server.\n";
        ws_server.stop();
        return 1;
    }

    std::cout << "Connected to ws://127.0.0.1:" << ws_server.port() << '\n';
    std::cout << "Lobby open for " << credentials.username << " (ELO " << user_record->elo
              << "). Click Play to join the queue. Press Esc to quit.\n";
    std::cout.flush();

    lobby.run(client);

    client.disconnect();
    ws_server.stop();

    return 0;
}
