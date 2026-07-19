#pragma once

#include <functional>
#include <optional>

#include "../model/piece.h"
#include "../model/position.h"

namespace engine {
class GameEngine;
}  // namespace engine

namespace input {

class Controller {
public:
    void handleClick(engine::GameEngine& gameEngine, int x, int y,
                     std::optional<model::Color> player_color = std::nullopt);
    void handleJump(engine::GameEngine& gameEngine, int x, int y,
                    std::optional<model::Color> player_color = std::nullopt);

    using MoveDispatch = std::function<bool(const model::Position&, const model::Position&)>;
    using JumpDispatch = std::function<bool(const model::Position&)>;

    void setDispatch(MoveDispatch move_dispatch, JumpDispatch jump_dispatch);

    bool hasSelection() const { return selected_square_.row >= 0; }
    const model::Position& selectedSquare() const { return selected_square_; }

private:
    void clearSelection();

    bool requestMove(engine::GameEngine& gameEngine, const model::Position& from,
                     const model::Position& to);
    bool requestJump(engine::GameEngine& gameEngine, const model::Position& square);

    MoveDispatch move_dispatch_;
    JumpDispatch jump_dispatch_;
    model::Position selected_square_{-1, -1};
};

}  // namespace input
