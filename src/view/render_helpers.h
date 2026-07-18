#pragma once

#include <opencv2/core.hpp>
#include <string>

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

}  // namespace view::render
