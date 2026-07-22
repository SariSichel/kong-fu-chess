#include "render_helpers.h"

#include "../config/game_config.h"
#include "../config/ui_config.h"
#include "disconnect_overlay.h"
#include "../engine/move_log.h"

#include <algorithm>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
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

float cooldownProgress(int remainingMs, int totalMs) {
    if (totalMs <= 0) {
        return 1.0f;
    }

    const float remaining = static_cast<float>(remainingMs);
    const float total = static_cast<float>(totalMs);
    return 1.0f - std::clamp(remaining / total, 0.0f, 1.0f);
}

void drawCooldownOverlay(cv::Mat& canvas,
                         float centerX,
                         float centerY,
                         int cellSize,
                         float progress,
                         int remainingMs,
                         const cv::Scalar& ringColor) {
    if (progress >= 1.0f || remainingMs <= 0) {
        return;
    }

    const int radius = std::max(8, cellSize * 42 / 100);
    const cv::Point center(static_cast<int>(centerX), static_cast<int>(centerY));

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy > radius * radius) {
                continue;
            }

            const int x = center.x + dx;
            const int y = center.y + dy;
            if (x < 0 || y < 0 || x >= canvas.cols || y >= canvas.rows) {
                continue;
            }

            cv::Vec3b& pixel = canvas.at<cv::Vec3b>(y, x);
            pixel[0] = cv::saturate_cast<uchar>(pixel[0] * 0.55f + 20.0f * 0.45f);
            pixel[1] = cv::saturate_cast<uchar>(pixel[1] * 0.55f + 20.0f * 0.45f);
            pixel[2] = cv::saturate_cast<uchar>(pixel[2] * 0.55f + 30.0f * 0.45f);
        }
    }

    const double startAngle = -90.0;
    const double endAngle = startAngle + 360.0 * static_cast<double>(progress);
    cv::ellipse(canvas, center, cv::Size(radius, radius), 0.0, startAngle, endAngle, ringColor, 3,
                cv::LINE_AA);

    const int seconds = (remainingMs + 999) / 1000;
    if (seconds > 0) {
        const std::string label = std::to_string(seconds);
        const double fontScale = 0.45;
        const int thickness = 1;
        int baseline = 0;
        const cv::Size textSize =
            cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, fontScale, thickness, &baseline);
        const cv::Point textOrigin(center.x - textSize.width / 2,
                                   center.y + textSize.height / 2);
        cv::putText(canvas, label, textOrigin, cv::FONT_HERSHEY_SIMPLEX, fontScale,
                    cv::Scalar(255, 255, 255), thickness, cv::LINE_AA);
    }
}

cv::Mat createExtendedCanvas(const cv::Mat& boardCanvas, int hudPanelWidth) {
    cv::Mat extended(boardCanvas.rows, boardCanvas.cols + hudPanelWidth, CV_8UC3,
                     cv::Scalar(28, 28, 32));
    boardCanvas.copyTo(extended(cv::Rect(0, 0, boardCanvas.cols, boardCanvas.rows)));

    const cv::Rect panelRect(boardCanvas.cols, 0, hudPanelWidth, boardCanvas.rows);
    cv::Mat panel = extended(panelRect);
    panel.setTo(cv::Scalar(36, 34, 42));

    return extended;
}

void drawTextLine(cv::Mat& canvas,
                  const std::string& text,
                  int x,
                  int y,
                  double fontScale,
                  const cv::Scalar& color,
                  int thickness) {
    cv::putText(canvas, text, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX, fontScale, color,
                thickness, cv::LINE_AA);
}

void drawDisconnectOverlay(cv::Mat& canvas, const view::DisconnectOverlayState& overlay) {
    if (!overlay.active) {
        return;
    }

    const int boardWidth = GameConfig::kBoardOriginX * 2 + GameConfig::kClickCellSize * 8;
    const int boardHeight = GameConfig::kBoardOriginY * 2 + GameConfig::kClickCellSize * 8;
    const cv::Rect boardRect(0, 0, boardWidth, boardHeight);

    cv::Mat overlayLayer = canvas(boardRect);
    overlayLayer = overlayLayer * 0.45 + cv::Scalar(20, 20, 30) * 0.55;

    const std::string line1 = "Opponent disconnected";
    const std::string line2 =
        "Reconnecting in " + std::to_string(overlay.seconds_remaining) + "s";

    const double titleScale = 0.9;
    const double bodyScale = 0.7;
    const cv::Scalar textColor(240, 240, 240);

    int baseline = 0;
    const cv::Size titleSize =
        cv::getTextSize(line1, cv::FONT_HERSHEY_SIMPLEX, titleScale, 2, &baseline);
    const cv::Size bodySize =
        cv::getTextSize(line2, cv::FONT_HERSHEY_SIMPLEX, bodyScale, 2, &baseline);

    const int centerX = boardWidth / 2;
    const int centerY = boardHeight / 2;
    const cv::Point titleOrigin(centerX - titleSize.width / 2, centerY - 8);
    const cv::Point bodyOrigin(centerX - bodySize.width / 2, centerY + titleSize.height + 8);

    drawTextLine(canvas, line1, titleOrigin.x, titleOrigin.y, titleScale, textColor, 2);
    drawTextLine(canvas, line2, bodyOrigin.x, bodyOrigin.y, bodyScale, textColor, 2);
}

void drawHudPanel(cv::Mat& canvas, int boardWidth, const engine::MoveLog& moveLog) {
    const int panelX = boardWidth + HudConfig::kPadding;
    int cursorY = HudConfig::kPadding + HudConfig::kLineHeight;

    const double titleScale =
        static_cast<double>(HudConfig::kTitleFontScaleTimes100) / 100.0;
    const double scoreScale =
        static_cast<double>(HudConfig::kScoreFontScaleTimes100) / 100.0;
    const double bodyScale = static_cast<double>(HudConfig::kBodyFontScaleTimes100) / 100.0;

    drawTextLine(canvas, "Score", panelX, cursorY, titleScale, cv::Scalar(220, 220, 220),
                 HudConfig::kTextThickness);
    cursorY += HudConfig::kLineHeight + 4;

    std::ostringstream scoreLine;
    scoreLine << "White: " << moveLog.whiteScore() << "   Black: " << moveLog.blackScore();
    drawTextLine(canvas, scoreLine.str(), panelX, cursorY, scoreScale, cv::Scalar(240, 240, 240),
                 HudConfig::kTextThickness);
    cursorY += HudConfig::kLineHeight + 12;

    drawTextLine(canvas, "Move Log", panelX, cursorY, titleScale, cv::Scalar(220, 220, 220),
                 HudConfig::kTextThickness);
    cursorY += HudConfig::kLineHeight + 6;

    const std::vector<engine::MoveLogEntry>& entries = moveLog.entries();
    const int startIndex =
        std::max(0, static_cast<int>(entries.size()) - HudConfig::kMaxMoveLogLines);

    if (startIndex >= static_cast<int>(entries.size())) {
        drawTextLine(canvas, "(no moves yet)", panelX, cursorY, bodyScale,
                     cv::Scalar(160, 160, 170), HudConfig::kTextThickness);
        return;
    }

    for (int index = startIndex; index < static_cast<int>(entries.size()); ++index) {
        const std::string& line = entries[static_cast<size_t>(index)].text;
        drawTextLine(canvas, line, panelX, cursorY, bodyScale, cv::Scalar(210, 210, 220),
                     HudConfig::kTextThickness);
        cursorY += HudConfig::kLineHeight;

        if (cursorY + HudConfig::kLineHeight > canvas.rows - HudConfig::kPadding) {
            break;
        }
    }
}

}  // namespace view::render
