#pragma once

#include <opencv2/core.hpp>
#include <string>
#include <vector>

namespace engine {
class MoveLog;
}  // namespace engine

namespace view {
struct DisconnectOverlayState;
}  // namespace view

namespace view::render {

constexpr int kMinVisibleAlpha = 16;

int centeredDrawCoord(int cellStart, int cellSize, int spriteSize);

float motionProgress(int startedAtMs, int durationMs, int elapsedMs);

cv::Mat loadBoardImage(const std::string& boardImagePath);

cv::Mat loadSpriteResized(const std::string& path, int targetW, int targetH);

void blitSpriteWithAlpha(cv::Mat& canvas, const cv::Mat& sprite, int x, int y);

void blitSpriteCentered(cv::Mat& canvas, const cv::Mat& sprite, float centerX, float centerY);

float cooldownProgress(int remainingMs, int totalMs);

void drawCooldownOverlay(cv::Mat& canvas,
                         float centerX,
                         float centerY,
                         int cellSize,
                         float progress,
                         int remainingMs,
                         const cv::Scalar& ringColor);

cv::Mat createExtendedCanvas(const cv::Mat& boardCanvas, int hudPanelWidth);

void drawTextLine(cv::Mat& canvas,
                  const std::string& text,
                  int x,
                  int y,
                  double fontScale,
                  const cv::Scalar& color,
                  int thickness = 1);

void drawHudPanel(cv::Mat& canvas, int boardWidth, const engine::MoveLog& moveLog);

void drawDisconnectOverlay(cv::Mat& canvas, const view::DisconnectOverlayState& overlay);

}  // namespace view::render
