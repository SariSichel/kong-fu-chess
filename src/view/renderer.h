#pragma once

#include <opencv2/core.hpp>
#include <string>

namespace engine {
class GameEngine;
}  // namespace engine

namespace input {
class Controller;
}  // namespace input

namespace model {
class Board;
}  // namespace model

namespace view {

class Renderer {
public:
    static constexpr const char* kWindowName = "Image";

    void init(const std::string& boardImagePath);
    void drawFrame(const engine::GameEngine& gameEngine, const input::Controller& controller);

    void render(const engine::GameEngine& gameEngine, const input::Controller& controller,
                const std::string& boardImagePath);

private:
    void drawPieces(cv::Mat& canvas, const model::Board& board);
    void drawSelectionOverlay(cv::Mat& canvas, const input::Controller& controller);

    cv::Mat board_canvas_;
};

}  // namespace view
