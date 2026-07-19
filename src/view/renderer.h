#pragma once

#include <opencv2/core.hpp>
#include <string>
#include <unordered_map>

#include "../constants.h"

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
    class SpriteCache {
    public:
        explicit SpriteCache(int cellSize);

        const cv::Mat& idleSpriteFor(const model::Piece& piece);

    private:
        int cell_size_;
        std::unordered_map<std::string, cv::Mat> sprites_;
    };

    void drawScene(cv::Mat& canvas, const engine::GameEngine& gameEngine);
    void drawPieceAtCell(cv::Mat& canvas, const model::Piece& piece, int row, int col);
    void drawMotion(cv::Mat& canvas, const realtime::Motion& motion, int elapsedMs);
    void drawSelectionOverlay(cv::Mat& canvas, const input::Controller& controller);

    cv::Mat board_canvas_;
    SpriteCache sprite_cache_{GameConfig::kClickCellSize};
};

}  // namespace view
