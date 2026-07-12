#pragma once

#include "../model/board.h"
#include "../model/position.h"
#include "piece_rules.h"

namespace rules {

class RuleEngine {
public:
    static bool validateMove(const model::Board& board, const model::Position& from, const model::Position& to);
};

}  // namespace rules
