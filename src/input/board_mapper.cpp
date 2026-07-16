#include "board_mapper.h"

#include "../constants.h"

namespace input {

namespace {

int cellIndex(int pixel, int origin, int cellSize) {
    const int adjusted = pixel - origin;
    if (adjusted < 0) {
        return -1;
    }
    return adjusted / cellSize;
}

}  // namespace

model::Position BoardMapper::toPosition(int x, int y) {
    if (x < 0 || y < 0) {
        return model::Position(-1, -1);
    }

    const int cellSize = GameConfig::kClickCellSize;
    const int row = cellIndex(y, GameConfig::kBoardOriginY, cellSize);
    const int col = cellIndex(x, GameConfig::kBoardOriginX, cellSize);
    if (row < 0 || col < 0) {
        return model::Position(-1, -1);
    }

    return model::Position(row, col);
}

void BoardMapper::toPixelCenter(int row, int col, float& centerX, float& centerY) {
    const int cellSize = GameConfig::kClickCellSize;
    centerX = static_cast<float>(GameConfig::kBoardOriginX + col * cellSize) + cellSize / 2.0f;
    centerY = static_cast<float>(GameConfig::kBoardOriginY + row * cellSize) + cellSize / 2.0f;
}

}  // namespace input
