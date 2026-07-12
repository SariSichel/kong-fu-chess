#include "board_mapper.h"

#include "../constants.h"

namespace input {

namespace {

int cellIndex(int pixel, int cellSize) {
    return (pixel - cellSize / 2) / cellSize;
}

}  // namespace

model::Position BoardMapper::toPosition(int x, int y) {
    if (x < 0 || y < 0) {
        return model::Position(-1, -1);
    }

    const int cellSize = GameConfig::kClickCellSize;
    return model::Position(cellIndex(y, cellSize), cellIndex(x, cellSize));
}

}  // namespace input
