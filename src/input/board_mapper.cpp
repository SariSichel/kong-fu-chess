#include "board_mapper.h"

#include "../constants.h"

namespace input {

model::Position BoardMapper::toPosition(int x, int y) {
    if (x < 0 || y < 0) {
        return model::Position(-1, -1);
    }
    return model::Position(y / GameConfig::kClickCellSize, x / GameConfig::kClickCellSize);
}

}  // namespace input
