#include "renderer.h"

#include <algorithm>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <string>

#include "../constants.h"
#include "../engine/game_engine.h"
#include "../input/board_mapper.h"
#include "../input/controller.h"
#include "../model/piece.h"
#include "../model/position.h"
#include "../realtime/motion.h"
#include "disconnect_overlay.h"
#include "piece_sprite.h"
#include "render_helpers.h"

namespace view {

namespace {

constexpr int kJumpLiftPx = 28;

bool isActiveMotionSource(const std::vector<realtime::Motion>& motions,
                          const model::Position& pos) {
    return std::any_of(motions.begin(), motions.end(), [&](const realtime::Motion& motion) {
        return motion.source == pos;
    });
}

model::Piece motionPiece(const realtime::Motion& motion) {
    return model::Piece(motion.piece_type, motion.piece_color);
}

void drawPieceCooldownOverlay(cv::Mat& canvas,
                              const model::Piece& piece,
                              float centerX,
                              float centerY) {
    const int cellSize = GameConfig::kClickCellSize;

    if (piece.moveCooldownRemainingMs() > 0) {
        const float progress = render::cooldownProgress(piece.moveCooldownRemainingMs(),
                                                        piece.moveCooldownTotalMs());
        render::drawCooldownOverlay(canvas, centerX, centerY, cellSize, progress,
                                    piece.moveCooldownRemainingMs(), cv::Scalar(0, 180, 255));
        return;
    }

    if (piece.jumpCooldownRemainingMs() > 0) {
        const float progress = render::cooldownProgress(piece.jumpCooldownRemainingMs(),
                                                        piece.jumpCooldownTotalMs());
        render::drawCooldownOverlay(canvas, centerX, centerY, cellSize, progress,
                                    piece.jumpCooldownRemainingMs(), cv::Scalar(255, 140, 0));
    }
}

}  // namespace

Renderer::SpriteCache::SpriteCache(int cellSize) : cell_size_(cellSize) {}

const cv::Mat& Renderer::SpriteCache::idleSpriteFor(const model::Piece& piece) {
    const std::string code = pieceToSpriteCode(piece);
    const auto it = sprites_.find(code);
    if (it != sprites_.end()) {
        return it->second;
    }

    const cv::Mat loaded =
        render::loadSpriteResized(idleSpritePath(code), cell_size_, cell_size_);
    const auto inserted = sprites_.emplace(code, std::move(loaded));
    return inserted.first->second;
}

void Renderer::init(const std::string& boardImagePath) {
    board_canvas_ = render::loadBoardImage(boardImagePath);
}

void Renderer::drawFrame(const engine::GameEngine& gameEngine,
                         const input::Controller& controller, const char* window_name,
                         const DisconnectOverlayState* overlay) {
    if (board_canvas_.empty()) {
        throw std::runtime_error("Renderer not initialized; call init() first.");
    }

    cv::Mat canvas =
        render::createExtendedCanvas(board_canvas_, HudConfig::kPanelWidth);
    drawScene(canvas, gameEngine);
    drawSelectionOverlay(canvas, controller);
    drawHud(canvas, board_canvas_.cols, gameEngine);
    if (overlay != nullptr) {
        render::drawDisconnectOverlay(canvas, *overlay);
    }
    cv::imshow(window_name, canvas);
}

void Renderer::render(const engine::GameEngine& gameEngine, const input::Controller& controller,
                      const std::string& boardImagePath) {
    init(boardImagePath);
    drawFrame(gameEngine, controller, kWhiteWindowName);
    cv::waitKey(0);
    cv::destroyAllWindows();
}

void Renderer::drawScene(cv::Mat& canvas, const engine::GameEngine& gameEngine) {
    const model::Board& board = gameEngine.board();
    const std::vector<realtime::Motion>& motions = gameEngine.activeMotions();
    const int elapsedMs = gameEngine.elapsedMs();

    for (size_t row = 0; row < board.rows(); ++row) {
        for (size_t col = 0; col < board.cols(); ++col) {
            const model::Position square{static_cast<int>(row), static_cast<int>(col)};
            const model::Piece& piece = board.cell(square);
            if (piece.isEmpty() || isActiveMotionSource(motions, square)) {
                continue;
            }

            drawPieceAtCell(canvas, piece, static_cast<int>(row), static_cast<int>(col));
        }
    }

    for (const realtime::Motion& motion : motions) {
        drawMotion(canvas, motion, elapsedMs);
    }
}

void Renderer::drawPieceAtCell(cv::Mat& canvas, const model::Piece& piece, int row, int col) {
    const cv::Mat& sprite = sprite_cache_.idleSpriteFor(piece);

    float centerX = 0.0f;
    float centerY = 0.0f;
    input::BoardMapper::toPixelCenter(row, col, centerX, centerY);
    render::blitSpriteCentered(canvas, sprite, centerX, centerY);
    drawPieceCooldownOverlay(canvas, piece, centerX, centerY);
}

void Renderer::drawMotion(cv::Mat& canvas, const realtime::Motion& motion, int elapsedMs) {
    const model::Piece piece = motionPiece(motion);
    const cv::Mat& sprite = sprite_cache_.idleSpriteFor(piece);

    const float progress =
        render::motionProgress(motion.started_at_ms, motion.duration_ms, elapsedMs);

    float centerX = 0.0f;
    float centerY = 0.0f;

    if (motion.type == realtime::MotionType::Move) {
        float sourceX = 0.0f;
        float sourceY = 0.0f;
        float destX = 0.0f;
        float destY = 0.0f;
        input::BoardMapper::toPixelCenter(motion.source.row, motion.source.col, sourceX, sourceY);
        input::BoardMapper::toPixelCenter(motion.destination.row, motion.destination.col, destX,
                                          destY);
        centerX = sourceX + (destX - sourceX) * progress;
        centerY = sourceY + (destY - sourceY) * progress;
    } else {
        input::BoardMapper::toPixelCenter(motion.source.row, motion.source.col, centerX, centerY);
        const float lift = 4.0f * progress * (1.0f - progress) * static_cast<float>(kJumpLiftPx);
        centerY -= lift;
    }

    render::blitSpriteCentered(canvas, sprite, centerX, centerY);
}

void Renderer::drawHud(cv::Mat& canvas, int boardWidth, const engine::GameEngine& gameEngine) {
    render::drawHudPanel(canvas, boardWidth, gameEngine.moveLog());
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
    const cv::Scalar highlightColor(0, 220, 255);
    cv::rectangle(canvas, cellRect, highlightColor, kBorderThickness);
}

}  // namespace view
