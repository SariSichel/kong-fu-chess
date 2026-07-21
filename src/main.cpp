//git link:
//https://github.com/SariSichel/kong-fu-chess/tree/main

#include "db/user_store.h"
#include "elo/elo_rating.h"
#include "engine/game_engine.h"
#include "events/event_bus.h"
#include "input/controller.h"
#include "io/board_setup.h"
#include "network/client_game_sync.h"
#include "network/command_queue.h"
#include "network/local_ws_client.h"
#include "network/network_input.h"
#include "network/ws_protocol.h"
#include "network/ws_server.h"
#include "session/session_manager.h"
#include "session/shell_login.h"
#include "session/shell_lobby.h"
#include "view/renderer.h"

#include "constants.h"
#include <opencv2/highgui.hpp>

#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <variant>
#include <vector>

namespace {

constexpr int kFrameMs = 16;

struct PlayerWindowContext {
    engine::GameEngine* game_engine = nullptr;
    input::Controller* controller = nullptr;
    std::optional<model::Color> player_color;
    bool game_started = false;
};

void onPlayerMouse(int event, int x, int y, int /*flags*/, void* userdata) {
    auto* context = static_cast<PlayerWindowContext*>(userdata);
    if (context == nullptr || context->game_engine == nullptr || context->controller == nullptr ||
        !context->game_started) {
        return;
    }

    if (event == cv::EVENT_LBUTTONDOWN) {
        context->controller->handleClick(*context->game_engine, x, y, context->player_color);
    } else if (event == cv::EVENT_RBUTTONDOWN) {
        context->controller->handleJump(*context->game_engine, x, y, context->player_color);
    }
}

void processNetworkCommands(network::CommandQueue& command_queue,
                            network::NetworkInputHandler& network_input) {
    std::vector<network::QueuedCommand> pending;
    command_queue.drain(pending);

    for (const network::QueuedCommand& queued : pending) {
        if (std::holds_alternative<network::LoginCommand>(queued.command) ||
            std::holds_alternative<network::JoinQueueCommand>(queued.command) ||
            std::holds_alternative<network::LeaveQueueCommand>(queued.command)) {
            continue;
        }

        if (const auto* move = std::get_if<network::MoveCommand>(&queued.command)) {
            network_input.handleMove(queued.connection_id, move->from, move->to);
            continue;
        }

        if (const auto* jump = std::get_if<network::JumpCommand>(&queued.command)) {
            network_input.handleJump(queued.connection_id, jump->square);
        }
    }
}

const char* windowNameForColor(model::Color color) {
    return color == model::Color::White ? view::Renderer::kWhiteWindowName
                                        : view::Renderer::kBlackWindowName;
}

void runGameLoop(engine::GameEngine& game_engine, input::Controller& controller,
                 view::Renderer& renderer, network::CommandQueue& command_queue,
                 network::NetworkInputHandler& network_input, model::Color player_color,
                 bool is_server_host) {
    renderer.init(AssetPaths::kBoardImage);

    const char* window_name = windowNameForColor(player_color);
    cv::namedWindow(window_name);
    cv::moveWindow(window_name, player_color == model::Color::White ? 40 : 960, 40);

    PlayerWindowContext context{&game_engine, &controller, player_color, true};
    cv::setMouseCallback(window_name, onPlayerMouse, &context);

    bool running = true;
    while (running && !game_engine.isGameOver()) {
        if (is_server_host) {
            processNetworkCommands(command_queue, network_input);
        }

        game_engine.advanceTime(kFrameMs);
        renderer.drawFrame(game_engine, controller, window_name);

        const int key = cv::waitKey(kFrameMs);
        if (key == 27) {
            running = false;
        }
    }

    cv::destroyWindow(window_name);
}

}  // namespace

int main(int argc, char* argv[]) {
    bool force_client = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--client") {
            force_client = true;
        }
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

    if (!force_client) {
        ws_server = std::make_unique<network::WsServer>(session, bus, store, command_queue,
                                                        NetworkConfig::kDefaultPort);
        is_server_host = ws_server->start();
        if (!is_server_host) {
            ws_server.reset();
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

    std::cout << "Connected to ws://127.0.0.1:" << server_port << '\n';
    if (is_server_host) {
        std::cout << "Hosting matchmaking server.\n";
    } else {
        std::cout << "Running in client-only mode.\n";
    }
    std::cout << "Lobby open for " << credentials.username << " (ELO " << user_record->elo
              << "). Click Play to find a match. Press Esc to quit.\n";
    std::cout.flush();

    const std::optional<session::MatchInfo> match = lobby.run(client);
    if (!match.has_value()) {
        client.disconnect();
        if (ws_server) {
            ws_server->stop();
        }
        return 0;
    }

    bus.subscribe([&store, match](const events::GameEvent& event) {
        const auto* ended = std::get_if<events::GameEnded>(&event);
        if (ended == nullptr) {
            return;
        }

        const bool white_won = ended->winner == model::Color::White;
        const std::string& winner_name =
            white_won ? match->white_user : match->black_user;
        const std::string& loser_name = white_won ? match->black_user : match->white_user;

        const std::optional<db::UserRecord> winner_record = store.find(winner_name);
        const std::optional<db::UserRecord> loser_record = store.find(loser_name);
        if (!winner_record.has_value() || !loser_record.has_value()) {
            return;
        }

        const elo::EloUpdate update =
            elo::computeUpdate(winner_record->elo, loser_record->elo);
        store.updateElo(winner_name, update.winner_new);
        store.updateElo(loser_name, update.loser_new);

        std::cout << winner_name << " wins! New ELO - " << winner_name << ": "
                  << update.winner_new << ", " << loser_name << ": " << update.loser_new << '\n';
    });

    engine::GameEngine game_engine;
    game_engine.setEventBus(is_server_host ? &bus : nullptr);
    io::setupStandardBoard(game_engine.board());
    game_engine.reset();

    input::Controller controller;
    network::NetworkInputHandler network_input(session, game_engine);
    controller.setDispatch(
        [&client](const model::Position& from, const model::Position& to) {
            return client.sendMove(from, to);
        },
        [&client](const model::Position& square) { return client.sendJump(square); });

    if (!is_server_host) {
        client.setMessageHandler([&game_engine](const std::string& json) {
            network::ClientGameSync::applyMessage(json, game_engine);
        });
    } else {
        client.setMessageHandler([](const std::string& /*json*/) {});
    }

    view::Renderer renderer;

    std::cout << "Match found! You are "
              << (match->color == model::Color::White ? "White" : "Black") << " vs "
              << match->opponent << ".\n";
    std::cout.flush();

    runGameLoop(game_engine, controller, renderer, command_queue, network_input, match->color,
                is_server_host);

    client.disconnect();
    if (ws_server) {
        ws_server->stop();
    }

    return 0;
}
