#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

namespace BoardTokens {
constexpr char kEmpty = '.';
constexpr const char* kColors = "wb";
constexpr const char* kPieceTypes = "KQRBNP";

constexpr char kKing = 'K';
constexpr char kQueen = 'Q';
constexpr char kRook = 'R';
constexpr char kBishop = 'B';
constexpr char kKnight = 'N';
constexpr char kPawn = 'P';
}  // namespace BoardTokens

namespace InputMarkers {
constexpr const char* kBoardSection = "Board:";
constexpr const char* kCommandsSection = "Commands:";
}  // namespace InputMarkers

namespace Commands {
constexpr const char* kClick = "click";
constexpr const char* kJump = "jump";
constexpr const char* kWait = "wait";
constexpr const char* kPrint = "print";
constexpr const char* kPrintBoard = "board";
}  // namespace Commands

namespace ErrorMessages {
constexpr const char* kUnknownToken = "ERROR UNKNOWN_TOKEN";
constexpr const char* kRowWidthMismatch = "ERROR ROW_WIDTH_MISMATCH";
}  // namespace ErrorMessages

namespace GameConfig {
constexpr int kClickCellSize = 100;
constexpr int kBoardOriginX = 11;
constexpr int kBoardOriginY = 14;
constexpr int kNoSelection = -1;
constexpr std::int64_t kMoveDurationMs = 1000;
constexpr std::int64_t kJumpDurationMs = 2000;
constexpr std::int64_t kMoveCooldownMs = 1000;
constexpr std::int64_t kJumpCooldownMs = 2000;
}  // namespace GameConfig

namespace NetworkConfig {
constexpr int kDefaultPort = 8765;
constexpr int kMaxPlayers = 2;
}  // namespace NetworkConfig

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

namespace AssetPaths {
constexpr const char* kAssetsRoot =
    R"(C:\Users\saris\OneDrive\Documents\bootcamp\assets\assets)";
constexpr const char* kPiecesRoot =
    R"(C:\Users\saris\OneDrive\Documents\bootcamp\assets\assets\pieces_mine)";
constexpr const char* kBoardImage =
    R"(C:\Users\saris\OneDrive\Documents\bootcamp\assets\assets\board.png)";
constexpr const char* kIdleSpriteState = "idle";
constexpr const char* kIdleSpriteFile = "1.png";
}  // namespace AssetPaths

namespace Text {
constexpr const char* kWhitespace = " \t\r\n";
}  // namespace Text

#endif
