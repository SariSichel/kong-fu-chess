#pragma once

#include <vector>

#include "../model/board.h"
#include "../model/position.h"
#include "motion.h"
#include "move_resolution.h"

namespace realtime {

class RealTimeArbiter {
public:
    void addMotion(const Motion& motion);
    std::vector<MoveResolution> advanceTime(int ms, model::Board& board);
    const std::vector<Motion>& getActiveMotions() const;
    int elapsedMs() const { return elapsed_ms_; }
    void clear();

private:
    std::vector<Motion> active_motions_;
    int elapsed_ms_ = 0;
};

}  // namespace realtime
