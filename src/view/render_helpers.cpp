#include "render_helpers.h"

#include <algorithm>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <vector>

namespace view::render {

int centeredDrawCoord(int cellStart, int cellSize, int spriteSize) {
    return cellStart + (cellSize - spriteSize) / 2;
}

float motionProgress(int startedAtMs, int durationMs, int elapsedMs) {
    if (durationMs <= 0) {
        return 1.0f;
    }

    const float elapsed = static_cast<float>(elapsedMs - startedAtMs);
    const float duration = static_cast<float>(durationMs);
    return std::clamp(elapsed / duration, 0.0f, 1.0f);
}

cv::Mat loadBoardImage(const std::string& boardImagePath) {
    cv::Mat board = cv::imread(boardImagePath, cv::IMREAD_UNCHANGED);
    if (board.empty()) {
        throw std::runtime_error("Cannot load board image: " + boardImagePath);
    }

    if (board.channels() == 4) {
        cv::Mat bgr;
        cv::cvtColor(board, bgr, cv::COLOR_BGRA2BGR);
        board = bgr;
    } else if (board.channels() == 1) {
        cv::cvtColor(board, board, cv::COLOR_GRAY2BGR);
    }

    return board;
}

cv::Mat loadSpriteResized(const std::string& path, int targetW, int targetH) {
    cv::Mat src = cv::imread(path, cv::IMREAD_UNCHANGED);
    if (src.empty()) {
        throw std::runtime_error("Cannot load sprite: " + path);
    }

    const double scale = std::min(static_cast<double>(targetW) / src.cols,
                                  static_cast<double>(targetH) / src.rows);
    const int newW = std::max(1, static_cast<int>(src.cols * scale));
    const int newH = std::max(1, static_cast<int>(src.rows * scale));
    const cv::Size newSize(newW, newH);

    if (src.channels() == 4) {
        std::vector<cv::Mat> channels;
        cv::split(src, channels);
        cv::Mat bgr;
        cv::merge(std::vector<cv::Mat>{channels[0], channels[1], channels[2]}, bgr);
        cv::Mat alpha = channels[3];

        cv::resize(bgr, bgr, newSize, 0, 0, cv::INTER_AREA);
        cv::resize(alpha, alpha, newSize, 0, 0, cv::INTER_AREA);

        std::vector<cv::Mat> bgrChannels;
        cv::split(bgr, bgrChannels);
        cv::Mat result;
        cv::merge(std::vector<cv::Mat>{bgrChannels[0], bgrChannels[1], bgrChannels[2], alpha},
                  result);
        return result;
    }

    cv::resize(src, src, newSize, 0, 0, cv::INTER_AREA);
    return src;
}

void blitSpriteWithAlpha(cv::Mat& canvas, const cv::Mat& sprite, int x, int y) {
    if (sprite.empty() || canvas.empty()) {
        return;
    }

    const int h = sprite.rows;
    const int w = sprite.cols;
    if (x < 0 || y < 0 || x + w > canvas.cols || y + h > canvas.rows) {
        throw std::runtime_error("Sprite does not fit at the specified position.");
    }

    cv::Mat roi = canvas(cv::Rect(x, y, w, h));

    if (sprite.channels() == 4) {
        for (int row = 0; row < h; ++row) {
            for (int col = 0; col < w; ++col) {
                const cv::Vec4b& src = sprite.at<cv::Vec4b>(row, col);
                if (src[3] < kMinVisibleAlpha) {
                    continue;
                }

                const float alpha = src[3] / 255.0f;
                cv::Vec3b& dst = roi.at<cv::Vec3b>(row, col);
                for (int channel = 0; channel < 3; ++channel) {
                    dst[channel] = cv::saturate_cast<uchar>(src[channel] * alpha +
                                                            dst[channel] * (1.0f - alpha));
                }
            }
        }
        return;
    }

    if (sprite.channels() == 3) {
        sprite.copyTo(roi);
        return;
    }

    throw std::runtime_error("Unsupported sprite channel count.");
}

void blitSpriteCentered(cv::Mat& canvas, const cv::Mat& sprite, float centerX, float centerY) {
    const int x = static_cast<int>(centerX - sprite.cols / 2.0f);
    const int y = static_cast<int>(centerY - sprite.rows / 2.0f);
    blitSpriteWithAlpha(canvas, sprite, x, y);
}

}  // namespace view::render
