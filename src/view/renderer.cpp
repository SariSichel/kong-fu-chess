#include "renderer.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <string>

#include "../constants.h"
#include "../engine/game_engine.h"
#include "../input/controller.h"
#include "../model/piece.h"
#include "../model/position.h"
#include "piece_sprite.h"
#include "render_helpers.h"

namespace view {

void Renderer::init(const std::string& boardImagePath) {
    board_canvas_ = render::loadBoardImage(boardImagePath);
}

void Renderer::drawFrame(const engine::GameEngine& gameEngine,
                         const input::Controller& controller) {
    if (board_canvas_.empty()) {
        throw std::runtime_error("Renderer not initialized; call init() first.");
    }

    cv::Mat canvas = board_canvas_.clone();
    drawPieces(canvas, gameEngine.board());
    drawSelectionOverlay(canvas, controller);
    cv::imshow(kWindowName, canvas);
}

void Renderer::render(const engine::GameEngine& gameEngine, const input::Controller& controller,
                      const std::string& boardImagePath) {
    init(boardImagePath);
    drawFrame(gameEngine, controller);
    cv::waitKey(0);
    cv::destroyAllWindows();
}

void Renderer::drawPieces(cv::Mat& canvas, const model::Board& board) {
    const int cellSize = GameConfig::kClickCellSize;
    const int originX = GameConfig::kBoardOriginX;
    const int originY = GameConfig::kBoardOriginY;

    for (size_t row = 0; row < board.rows(); ++row) {
        for (size_t col = 0; col < board.cols(); ++col) {
            const model::Piece& piece = board.cell(model::Position{static_cast<int>(row),
                                                                   static_cast<int>(col)});
            if (piece.isEmpty()) {
                continue;
            }

            const cv::Mat sprite =
                render::loadSpriteResized(idleSpritePath(pieceToSpriteCode(piece)), cellSize,
                                          cellSize);

            const int cellLeft = originX + static_cast<int>(col) * cellSize;
            const int cellTop = originY + static_cast<int>(row) * cellSize;
            const int x = render::centeredDrawCoord(cellLeft, cellSize, sprite.cols);
            const int y = render::centeredDrawCoord(cellTop, cellSize, sprite.rows);

            render::blitSpriteWithAlpha(canvas, sprite, x, y);
        }
    }
}

void Renderer::drawSelectionOverlay(cv::Mat& canvas, const input::Controller& controller) {
    if (!controller.hasSelection()) {
        return;
    }

    const model::Position& selected = controller.selectedSquare();
    const int cellSize = GameConfig::kClickCellSize;
    const int originX = GameConfig::kBoardOriginX;
    const int originY = GameConfig::kBoardOriginY;

    const int cellLeft = originX + selected.col * cellSize;
    const int cellTop = originY + selected.row * cellSize;
    const cv::Rect cellRect(cellLeft, cellTop, cellSize, cellSize);

    constexpr int kBorderThickness = 3;
    const cv::Scalar highlightColor(0, 220, 255);  // BGR: amber/yellow
    cv::rectangle(canvas, cellRect, highlightColor, kBorderThickness);
}

}  // namespace view
