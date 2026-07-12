#pragma once

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

#include "../model/board.h"
#include "../model/position.h"
#include "../realtime/motion.h"
#include "../realtime/real_time_arbiter.h"

namespace engine {

class GameEngine {
public:
    void reset();
    bool isGameOver() const { return is_game_over_; }
    void advanceTime(int ms);

    bool requestMove(const model::Position& from, const model::Position& to);
    bool requestJump(const model::Position& pos);

    bool hasPremoveAt(const model::Position& source) const;
    bool isActiveMoveStartTick(const model::Position& from) const;
    bool isBusyAt(const model::Position& pos) const;
    bool getActiveMoveDestination(const model::Position& from, model::Position& destination) const;

    void queuePremove(const model::Position& key, const model::Position& from,
                      const model::Position& to);

    model::Board& board() { return board_; }
    const model::Board& board() const { return board_; }

private:
    struct Premove {
        model::Position from;
        model::Position to;
    };

    using PremoveKey = std::pair<int, int>;

    static PremoveKey premoveKey(const model::Position& pos) {
        return {pos.row, pos.col};
    }

    void clearPremoveAt(const model::Position& source);
    void tryExecutePremove(const model::Position& moveSource, const model::Position& arrival);
    void checkGameOverFromCompletingMoves(const std::vector<realtime::Motion>& completingMoves);
    void processPremovesAfterAdvance(const std::vector<realtime::Motion>& completingMoves);

    model::Board board_;
    realtime::RealTimeArbiter arbiter_;
    bool is_game_over_ = false;
    std::map<PremoveKey, Premove> premoves_;
    uint64_t next_motion_id_ = 0;
};

}  // namespace engine
