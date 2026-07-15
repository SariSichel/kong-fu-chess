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
class Piece;
}  // namespace model

namespace realtime {
struct Motion;
}  // namespace realtime

namespace view {

class Renderer {
public:
    static constexpr const char* kWindowName = "Image";

    void init(const std::string& boardImagePath);
    void drawFrame(const engine::GameEngine& gameEngine, const input::Controller& controller);

    void render(const engine::GameEngine& gameEngine, const input::Controller& controller,
                const std::string& boardImagePath);

private:
    void drawScene(cv::Mat& canvas, const engine::GameEngine& gameEngine);
    void drawPieceAtCell(cv::Mat& canvas, const model::Piece& piece, int row, int col);
    void drawMotion(cv::Mat& canvas, const realtime::Motion& motion, int elapsedMs);
    void drawSelectionOverlay(cv::Mat& canvas, const input::Controller& controller);

    cv::Mat board_canvas_;
};

}  // namespace view
