#pragma once

#include <string>

namespace engine {
class GameEngine;
}  // namespace engine

namespace network {

class ClientGameSync {
public:
    static bool applyMessage(const std::string& json, engine::GameEngine& game_engine);
};

}  // namespace network
