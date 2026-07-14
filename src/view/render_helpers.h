#pragma once

#include <opencv2/core.hpp>
#include <string>

namespace view::render {

constexpr int kMinVisibleAlpha = 16;

int centeredDrawCoord(int cellStart, int cellSize, int spriteSize);

cv::Mat loadBoardImage(const std::string& boardImagePath);

cv::Mat loadSpriteResized(const std::string& path, int targetW, int targetH);

void blitSpriteWithAlpha(cv::Mat& canvas, const cv::Mat& sprite, int x, int y);

}  // namespace view::render
