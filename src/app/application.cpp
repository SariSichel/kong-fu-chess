#include "application.h"

#include "game_loop.h"
#include "host_election.h"
#include "net_system_guard.h"

#include "config/network_config.h"
#include "db/user_store.h"
#include "elo/elo_rating.h"
#include "engine/game_engine.h"
#include "events/event_bus.h"
#include "input/controller.h"
#include "io/board_setup.h"
#include "network/command_queue.h"
#include "network/local_ws_client.h"
#include "network/network_input.h"
#include "network/ws_server.h"
#include "online/session_manager.h"
#include "ui/lobby/shell_login.h"
#include "ui/lobby/shell_lobby.h"
#include "view/disconnect_overlay.h"
#include "view/renderer.h"

#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

namespace app {

Application::Application(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--client") {
            force_client_ = true;
        }
    }
}

int Application::run() {
    NetSystemGuard net_system;
    if (!net_system.ok()) {
        std::cerr << "Failed to initialize networking.\n";
        return 1;
    }

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
    session::SessionManager session(NetworkConfig::kDefaultPort);
    network::CommandQueue command_queue;

    std::unique_ptr<network::WsServer> ws_server;
    bool is_server_host = false;

    if (!force_client_ && acquireHostElection()) {
        ws_server = std::make_unique<network::WsServer>(session, bus, store, command_queue,
                                                        NetworkConfig::kDefaultPort);
        is_server_host = ws_server->start();
        if (!is_server_host) {
            ws_server.reset();
            releaseHostElection();
        }
    }

    const int server_port = NetworkConfig::kDefaultPort;
    std::ostringstream client_url;
    client_url << "ws://127.0.0.1:" << server_port;
    network::LocalWsClient client(client_url.str(), credentials.username);

    session::ShellLobby lobby(credentials.username, user_record->elo);
    client.setMessageHandler([&lobby](const std::string& json) { lobby.handleServerMessage(json); });

    if (!client.connectAndLogin()) {
        std::cerr << "Failed to connect to WebSocket server.\n";
        if (ws_server) {
            ws_server->stop();
        }
        return 1;
    }

    const bool reconnecting_to_game = client.assignedColor().has_value();

    std::cout << "Connected to ws://127.0.0.1:" << server_port << '\n';
    if (is_server_host) {
        std::cout << "Hosting matchmaking server.\n";
    } else {
        std::cout << "Running in client-only mode.\n";
    }
    if (reconnecting_to_game) {
        std::cout << "Reconnecting to active game as "
                  << (*client.assignedColor() == model::Color::White ? "White" : "Black")
                  << "...\n";
    } else {
        std::cout << "Lobby open for " << credentials.username << " (ELO " << user_record->elo
                  << "). Click Play to find a match. Press Esc to quit.\n";
    }
    std::cout.flush();

    const std::optional<session::MatchInfo> match = lobby.run(
        client, [&ws_server]() {
            if (ws_server) {
                ws_server->tick();
            }
        });
    if (!match.has_value()) {
        client.disconnect();
        if (ws_server) {
            ws_server->stop();
        }
        return 0;
    }

    engine::GameEngine game_engine;
    game_engine.setEventBus(is_server_host ? &bus : nullptr);
    io::setupStandardBoard(game_engine.board());
    game_engine.reset();

    bus.subscribe<events::GameEnded>([&store, &game_engine, match](const events::GameEnded& ended) {
        if (!game_engine.isGameOver()) {
            game_engine.applyRemoteGameEnded(ended.winner);
        }

        const bool white_won = ended.winner == model::Color::White;
        const std::string& winner_name = white_won ? match->white_user : match->black_user;
        const std::string& loser_name = white_won ? match->black_user : match->white_user;

        const std::optional<db::UserRecord> winner_record = store.find(winner_name);
        const std::optional<db::UserRecord> loser_record = store.find(loser_name);
        if (!winner_record.has_value() || !loser_record.has_value()) {
            return;
        }

        const elo::EloUpdate update = elo::computeUpdate(winner_record->elo, loser_record->elo);
        store.updateElo(winner_name, update.winner_new);
        store.updateElo(loser_name, update.loser_new);

        std::cout << winner_name << " wins! New ELO - " << winner_name << ": " << update.winner_new
                  << ", " << loser_name << ": " << update.loser_new << '\n';
    });

    input::Controller controller;
    network::NetworkInputHandler network_input(session, game_engine);
    controller.setDispatch(
        [&client, &game_engine, is_server_host](const model::Position& from,
                                                 const model::Position& to) {
            if (!client.sendMove(from, to)) {
                return false;
            }

            if (!is_server_host) {
                game_engine.requestMove(from, to);
            }

            return true;
        },
        [&client, &game_engine, is_server_host](const model::Position& square) {
            if (!client.sendJump(square)) {
                return false;
            }

            if (!is_server_host) {
                game_engine.requestJump(square);
            }

            return true;
        });

    view::Renderer renderer;
    view::DisconnectOverlayState disconnect_overlay;

    std::cout << "Match found! You are "
              << (match->color == model::Color::White ? "White" : "Black") << " vs "
              << match->opponent << ".\n";
    std::cout.flush();

    GameLoop::run(game_engine, controller, renderer, command_queue, network_input, client,
                  disconnect_overlay, match->color, is_server_host, ws_server.get());

    client.disconnect();
    if (ws_server) {
        ws_server->stop();
    }

    return 0;
}

}  // namespace app
