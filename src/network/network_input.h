#pragma once

#include <cstdint>
#include <string>

#include "../model/position.h"
#include "../session/session_manager.h"

namespace engine {
class GameEngine;
}

namespace network {

class NetworkInputHandler {
public:
    NetworkInputHandler(session::SessionManager& session, engine::GameEngine& game_engine);

    session::AuthResult handleLogin(std::uint64_t connection_id, const std::string& username);
    bool handleMove(std::uint64_t connection_id, const model::Position& from,
                    const model::Position& to);
    bool handleJump(std::uint64_t connection_id, const model::Position& square);
    bool handleMove(std::uint64_t connection_id, const std::string& from,
                    const std::string& to);
    bool handleJump(std::uint64_t connection_id, const std::string& square);

private:
    bool canControlSquare(std::uint64_t connection_id, const model::Position& square) const;

    session::SessionManager& session_;
    engine::GameEngine& game_engine_;
};

}  // namespace network
