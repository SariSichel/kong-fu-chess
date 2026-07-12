#pragma once

#include "../model/position.h"

namespace engine {
class GameEngine;
}  // namespace engine

namespace input {

class Controller {
public:
    void handleClick(engine::GameEngine& gameEngine, int x, int y);
    void handleJump(engine::GameEngine& gameEngine, int x, int y);

    bool hasSelection() const { return selected_square_.row >= 0; }
    const model::Position& selectedSquare() const { return selected_square_; }

private:
    void clearSelection();

    model::Position selected_square_{-1, -1};
};

}  // namespace input
