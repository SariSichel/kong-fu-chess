#pragma once

#include "model/piece.h"

namespace engine {
class GameEngine;
}

namespace input {
class Controller;
}

namespace network {
class CommandQueue;
class LocalWsClient;
class NetworkInputHandler;
class WsServer;
}  // namespace network

namespace view {
class DisconnectOverlayState;
class Renderer;
}  // namespace view

namespace app {

class GameLoop {
public:
    static void run(engine::GameEngine& game_engine, input::Controller& controller,
                    view::Renderer& renderer, network::CommandQueue& command_queue,
                    network::NetworkInputHandler& network_input, network::LocalWsClient& client,
                    view::DisconnectOverlayState& disconnect_overlay, model::Color player_color,
                    bool is_server_host, network::WsServer* ws_server);
};

}  // namespace app
