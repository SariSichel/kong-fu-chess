#ifndef CONFIG_GAME_CONFIG_H
#define CONFIG_GAME_CONFIG_H

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

namespace Text {
constexpr const char* kWhitespace = " \t\r\n";
}  // namespace Text

#endif
