#pragma once

#include "../model/position.h"

namespace input {

class BoardMapper {
public:
    static model::Position toPosition(int x, int y);
};

}  // namespace input
