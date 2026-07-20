//git link:
//https://github.com/SariSichel/kong-fu-chess/tree/main

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
#include "session/session_manager.h"
#include "session/shell_login.h"
#include "view/renderer.h"

#include "constants.h"
#include <opencv2/highgui.hpp>

#include <iostream>
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
    bool* game_started = nullptr;
};

void onPlayerMouse(int event, int x, int y, int /*flags*/, void* userdata) {
    auto* context = static_cast<PlayerWindowContext*>(userdata);
    if (context == nullptr || context->game_engine == nullptr || context->controller == nullptr ||
        context->game_started == nullptr || !*context->game_started) {
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
        if (std::holds_alternative<network::LoginCommand>(queued.command)) {
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

void runNetworkGameLoop(engine::GameEngine& game_engine, input::Controller& white_controller,
                        input::Controller& black_controller, view::Renderer& renderer,
                        session::SessionManager& session, events::EventBus& bus,
                        network::CommandQueue& command_queue,
                        network::NetworkInputHandler& network_input) {
    renderer.init(AssetPaths::kBoardImage);
    cv::namedWindow(view::Renderer::kWhiteWindowName);
    cv::namedWindow(view::Renderer::kBlackWindowName);
    cv::moveWindow(view::Renderer::kWhiteWindowName, 40, 40);
    cv::moveWindow(view::Renderer::kBlackWindowName, 960, 40);

    bool game_started = false;
    bool running = true;

    PlayerWindowContext white_context{&game_engine, &white_controller, model::Color::White,
                                      &game_started};
    PlayerWindowContext black_context{&game_engine, &black_controller, model::Color::Black,
                                      &game_started};
    cv::setMouseCallback(view::Renderer::kWhiteWindowName, onPlayerMouse, &white_context);
    cv::setMouseCallback(view::Renderer::kBlackWindowName, onPlayerMouse, &black_context);

    while (running) {
        if (!game_started && session.isReady()) {
            bus.publish(session.rosterSnapshot());
            game_started = true;
            std::cout << "Both players connected. Game is live.\n";
            std::cout << "  White window: left-click move, right-click jump.\n";
            std::cout << "  Black window: left-click move, right-click jump.\n";
            std::cout.flush();
        }

        if (game_started) {
            processNetworkCommands(command_queue, network_input);
        }

        game_engine.advanceTime(kFrameMs);
        renderer.drawFrame(game_engine, white_controller, view::Renderer::kWhiteWindowName);
        renderer.drawFrame(game_engine, black_controller, view::Renderer::kBlackWindowName);

        const int key = cv::waitKey(kFrameMs);
        if (key == 27) {
            running = false;
        }
    }

    cv::destroyAllWindows();
}

}  // namespace

int main() {
    const session::TwoPlayerNames names =
        session::ShellLogin::readTwoPlayerNames(std::cin, std::cout);
    if (names.white_user.empty() || names.black_user.empty()) {
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

    if (!authenticateOrRegister(names.white_user, names.white_password)) {
        std::cerr << "Authentication failed for '" << names.white_user << "'.\n";
        return 1;
    }

    if (!authenticateOrRegister(names.black_user, names.black_password)) {
        std::cerr << "Authentication failed for '" << names.black_user << "'.\n";
        return 1;
    }

    events::EventBus bus;
    bus.subscribe([&store, &names](const events::GameEvent& event) {
        const auto* ended = std::get_if<events::GameEnded>(&event);
        if (ended == nullptr) {
            return;
        }

        const bool white_won = ended->winner == model::Color::White;
        const std::string& winner_name = white_won ? names.white_user : names.black_user;
        const std::string& loser_name = white_won ? names.black_user : names.white_user;

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
                  << update.winner_new << ", " << loser_name << ": " << update.loser_new
                  << '\n';
    });

    session::SessionManager session(names.white_user, names.black_user,
                                    NetworkConfig::kDefaultPort);

    network::CommandQueue command_queue;
    network::WsServer ws_server(session, bus, command_queue, NetworkConfig::kDefaultPort);

    if (!ws_server.start()) {
        std::cerr << "Failed to start WebSocket server on port " << ws_server.port() << ".\n";
        return 1;
    }

    std::ostringstream white_url;
    white_url << "ws://127.0.0.1:" << ws_server.port();
    network::LocalWsClient white_client(white_url.str(), names.white_user);
    network::LocalWsClient black_client(white_url.str(), names.black_user);

    if (!white_client.connectAndLogin()) {
        std::cerr << "Failed to connect White player to local WebSocket server.\n";
        ws_server.stop();
        return 1;
    }

    if (!black_client.connectAndLogin()) {
        std::cerr << "Failed to connect Black player to local WebSocket server.\n";
        white_client.disconnect();
        ws_server.stop();
        return 1;
    }

    std::cout << "Local WebSocket clients connected on ws://127.0.0.1:" << ws_server.port()
              << '\n';
    std::cout << "  White: " << names.white_user << " (" << view::Renderer::kWhiteWindowName
              << ")\n";
    std::cout << "  Black: " << names.black_user << " (" << view::Renderer::kBlackWindowName
              << ")\n";
    std::cout << "Left-click to move, right-click to jump. Press Esc to quit.\n";
    std::cout.flush();

    engine::GameEngine game_engine;
    game_engine.setEventBus(&bus);
    io::setupStandardBoard(game_engine.board());
    game_engine.reset();

    input::Controller white_controller;
    input::Controller black_controller;
    network::NetworkInputHandler network_input(session, game_engine);
    white_controller.setDispatch(
        [&white_client](const model::Position& from, const model::Position& to) {
            return white_client.sendMove(from, to);
        },
        [&white_client](const model::Position& square) { return white_client.sendJump(square); });
    black_controller.setDispatch(
        [&black_client](const model::Position& from, const model::Position& to) {
            return black_client.sendMove(from, to);
        },
        [&black_client](const model::Position& square) { return black_client.sendJump(square); });
    view::Renderer renderer;

    runNetworkGameLoop(game_engine, white_controller, black_controller, renderer, session, bus,
                       command_queue, network_input);

    white_client.disconnect();
    black_client.disconnect();

    ws_server.stop();

    return 0;
}
