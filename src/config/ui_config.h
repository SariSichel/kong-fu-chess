#ifndef CONFIG_UI_CONFIG_H
#define CONFIG_UI_CONFIG_H

namespace LobbyConfig {
constexpr int kWindowWidth = 480;
constexpr int kWindowHeight = 320;
constexpr int kPlayButtonX = 140;
constexpr int kPlayButtonY = 220;
constexpr int kPlayButtonWidth = 200;
constexpr int kPlayButtonHeight = 48;
constexpr int kFrameMs = 16;
}  // namespace LobbyConfig

namespace HudConfig {
constexpr int kPanelWidth = 280;
constexpr int kPadding = 12;
constexpr int kLineHeight = 22;
constexpr int kScoreFontScaleTimes100 = 55;
constexpr int kBodyFontScaleTimes100 = 40;
constexpr int kTitleFontScaleTimes100 = 45;
constexpr int kMaxMoveLogLines = 12;
constexpr int kTextThickness = 1;
}  // namespace HudConfig

#endif
