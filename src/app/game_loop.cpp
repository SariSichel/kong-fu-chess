#include "game_loop.h"

#include "config/paths.h"
#include "engine/game_engine.h"
#include "events/game_event.h"
#include "input/controller.h"
#include "network/client_game_sync.h"
#include "network/client_message_queue.h"
#include "network/command_queue.h"
#include "network/local_ws_client.h"
#include "network/network_input.h"
#include "network/ws_protocol.h"
#include "network/ws_server.h"
#include "view/disconnect_overlay.h"
#include "view/renderer.h"

#include <opencv2/highgui.hpp>

#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace app {

namespace {

constexpr int kFrameMs = 16;
constexpr int kGameOverDisplayMs = 2500;

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

void applyGameEndedMessage(const std::string& json, engine::GameEngine& game_engine) {
    const std::optional<events::GameEvent> event = network::parseServerGameEvent(json);
    if (!event.has_value()) {
        return;
    }

    if (const auto* ended = std::get_if<events::GameEnded>(&*event)) {
        if (!game_engine.isGameOver()) {
            game_engine.applyRemoteGameEnded(ended->winner);
        }
    }
}

void processServerMessages(const std::vector<std::string>& messages,
                           engine::GameEngine& game_engine,
                           view::DisconnectOverlayState& disconnect_overlay, bool is_server_host) {
    for (const std::string& json : messages) {
        const network::ServerMessageType message_type = network::parseServerMessageType(json);
        if (message_type == network::ServerMessageType::GameEnded) {
            applyGameEndedMessage(json, game_engine);
            disconnect_overlay.applyMessage(json);
            continue;
        }

        if (message_type == network::ServerMessageType::PlayerDisconnected ||
            message_type == network::ServerMessageType::PlayerReconnected) {
            disconnect_overlay.applyMessage(json);
            continue;
        }

        if (!is_server_host) {
            network::ClientGameSync::applyMessage(json, game_engine);
        }
    }
}

void showGameOverFeedback(const engine::GameEngine& game_engine, model::Color player_color,
                          const char* window_name, view::Renderer& renderer,
                          input::Controller& controller,
                          view::DisconnectOverlayState& disconnect_overlay) {
    if (!game_engine.isGameOver() || !game_engine.winner().has_value()) {
        return;
    }

    const bool player_won = *game_engine.winner() == player_color;
    std::cout << (player_won ? "You win!" : "You lose.") << '\n';
    std::cout.flush();

    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(kGameOverDisplayMs);
    while (std::chrono::steady_clock::now() < deadline) {
        renderer.drawFrame(game_engine, controller, window_name, &disconnect_overlay);
        cv::waitKey(kFrameMs);
    }
}

}  // namespace

void GameLoop::run(engine::GameEngine& game_engine, input::Controller& controller,
                   view::Renderer& renderer, network::CommandQueue& command_queue,
                   network::NetworkInputHandler& network_input, network::LocalWsClient& client,
                   view::DisconnectOverlayState& disconnect_overlay, model::Color player_color,
                   bool is_server_host, network::WsServer* ws_server) {
    renderer.init(AssetPaths::kBoardImage);

    const char* window_name = windowNameForColor(player_color);
    cv::namedWindow(window_name);
    cv::moveWindow(window_name, player_color == model::Color::White ? 40 : 960, 40);

    PlayerWindowContext context{&game_engine, &controller, player_color, true};
    cv::setMouseCallback(window_name, onPlayerMouse, &context);

    network::ClientMessageQueue message_queue;
    client.setMessageHandler(
        [&message_queue](const std::string& json) { message_queue.push(json); });

    std::vector<std::string> pending_messages;
    bool running = true;
    while (running && !game_engine.isGameOver()) {
        if (ws_server != nullptr) {
            ws_server->tick();
        }

        message_queue.drain(pending_messages);
        processServerMessages(pending_messages, game_engine, disconnect_overlay, is_server_host);

        if (is_server_host) {
            processNetworkCommands(command_queue, network_input);
        }

        disconnect_overlay.updateCountdown(std::chrono::steady_clock::now());
        game_engine.advanceTime(kFrameMs);
        renderer.drawFrame(game_engine, controller, window_name, &disconnect_overlay);

        const int key = cv::waitKey(kFrameMs);
        if (key == 27) {
            running = false;
        }
    }

    message_queue.drain(pending_messages);
    processServerMessages(pending_messages, game_engine, disconnect_overlay, is_server_host);

    showGameOverFeedback(game_engine, player_color, window_name, renderer, controller,
                         disconnect_overlay);
    cv::destroyWindow(window_name);
}

}  // namespace app
