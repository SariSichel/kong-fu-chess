#pragma once

#include <string>
#include <vector>

#include "../model/piece.h"
#include "../model/position.h"
#include "../realtime/move_resolution.h"

namespace engine {

struct MoveLogEntry {
    int timestamp_ms = 0;
    std::string text;
};

class MoveLog {
public:
    void clear();

    void recordCompletedMove(const realtime::CompletedMoveEvent& event);
    void recordJumpCapture(const realtime::JumpCaptureEvent& event);

    const std::vector<MoveLogEntry>& entries() const { return entries_; }

    int whiteScore() const { return white_score_; }
    int blackScore() const { return black_score_; }

    static int captureValue(model::PieceType type);
    static std::string formatElapsedMMSS(int timestampMs);
    static std::string positionToAlgebraic(const model::Position& pos);
    static std::string pieceLabel(model::PieceType type, model::Color color);

private:
    void addCaptureScore(model::Color scorer, model::PieceType victimType);

    std::vector<MoveLogEntry> entries_;
    int white_score_ = 0;
    int black_score_ = 0;
};

}  // namespace engine
