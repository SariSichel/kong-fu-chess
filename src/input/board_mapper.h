#pragma once

#include "../model/position.h"

namespace input {

class BoardMapper {
public:
    static model::Position toPosition(int x, int y);
    static void toPixelCenter(int row, int col, float& centerX, float& centerY);
};

}  // namespace input
