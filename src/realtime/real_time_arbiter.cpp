#include "real_time_arbiter.h"

#include "../constants.h"

#include <algorithm>
#include <vector>

namespace {

void decreaseAllCooldowns(model::Board& board, int ms) {
    for (size_t row = 0; row < board.rows(); ++row) {
        for (size_t col = 0; col < board.cols(); ++col) {
            board.cell(model::Position{static_cast<int>(row), static_cast<int>(col)}).decreaseCooldown(ms);
        }
    }
}

bool isJumpActiveAt(const std::vector<realtime::Motion>& motions, const model::Position& pos) {
    return std::any_of(motions.begin(), motions.end(), [&](const realtime::Motion& motion) {
        return motion.type == realtime::MotionType::Jump && motion.source == pos;
    });
}

bool squareArrivedThisTick(const std::vector<model::Position>& arrivedThisTick,
                           const model::Position& pos) {
    return std::any_of(arrivedThisTick.begin(), arrivedThisTick.end(),
                       [&](const model::Position& square) { return square == pos; });
}

void promotePawnIfEligible(model::Board& board, const model::Position& pos) {
    model::Piece& piece = board.cell(pos);
    if (piece.type() != model::PieceType::Pawn) {
        return;
    }

    const int lastRow =
        (piece.color() == model::Color::White) ? 0 : static_cast<int>(board.rows()) - 1;
    if (pos.row == lastRow) {
        piece.promote(model::PieceType::Queen);
    }
}

bool motionLess(const realtime::Motion& a, const realtime::Motion& b) {
    if (a.started_at_ms != b.started_at_ms) {
        return a.started_at_ms < b.started_at_ms;
    }
    return a.motion_id < b.motion_id;
}

void resolveMoveArrival(model::Board& board, const realtime::Motion& motion,
                        const std::vector<realtime::Motion>& activeMotions,
                        std::vector<model::Position>& arrivedThisTick) {
    if (!board.inBounds(motion.source)) {
        return;
    }

    model::Piece& mover = board.cell(motion.source);
    if (mover.isEmpty() || mover.type() != motion.piece_type || mover.color() != motion.piece_color) {
        return;
    }

    const model::Piece& destination = board.cell(motion.destination);
    if (!destination.isEmpty()) {
        if (destination.isFriendly(mover.color())) {
            return;
        }

        if (isJumpActiveAt(activeMotions, motion.destination)) {
            board.removePieceAt(motion.source);
            return;
        }

        if (squareArrivedThisTick(arrivedThisTick, motion.destination)) {
            board.removePieceAt(motion.source);
            return;
        }
    }

    board.movePiece(motion.source, motion.destination);
    arrivedThisTick.push_back(motion.destination);

    model::Piece& arrived = board.cell(motion.destination);
    if (arrived.type() == model::PieceType::Pawn) {
        arrived.markMoved();
    }
    promotePawnIfEligible(board, motion.destination);
}

void removeExpiredMotions(std::vector<realtime::Motion>& motions, realtime::MotionType type,
                          int currentTimeMs) {
    motions.erase(std::remove_if(motions.begin(), motions.end(),
                                 [type, currentTimeMs](const realtime::Motion& motion) {
                                     return motion.type == type && motion.isExpired(currentTimeMs);
                                 }),
                  motions.end());
}

}  // namespace

namespace realtime {

void RealTimeArbiter::addMotion(const Motion& motion) {
    active_motions_.push_back(motion);
}

void RealTimeArbiter::advanceTime(int ms, model::Board& board) {
    elapsed_ms_ += ms;
    decreaseAllCooldowns(board, ms);

    std::vector<Motion> completingMoves;
    completingMoves.reserve(active_motions_.size());
    for (const Motion& motion : active_motions_) {
        if (motion.type == MotionType::Move && motion.isExpired(elapsed_ms_)) {
            completingMoves.push_back(motion);
        }
    }

    if (!completingMoves.empty()) {
        std::sort(completingMoves.begin(), completingMoves.end(), motionLess);

        std::vector<model::Position> arrivedThisTick;
        arrivedThisTick.reserve(completingMoves.size());

        for (const Motion& motion : completingMoves) {
            resolveMoveArrival(board, motion, active_motions_, arrivedThisTick);
        }

        removeExpiredMotions(active_motions_, MotionType::Move, elapsed_ms_);
    }

    for (const Motion& motion : active_motions_) {
        if (motion.type != MotionType::Jump || !motion.isExpired(elapsed_ms_)) {
            continue;
        }

        if (!board.inBounds(motion.source)) {
            continue;
        }

        model::Piece& piece = board.cell(motion.source);
        if (!piece.isEmpty()) {
            piece.setJumpCooldown(static_cast<int>(GameConfig::kJumpCooldownMs));
        }
    }

    removeExpiredMotions(active_motions_, MotionType::Jump, elapsed_ms_);
}

const std::vector<Motion>& RealTimeArbiter::getActiveMotions() const {
    return active_motions_;
}

void RealTimeArbiter::clear() {
    active_motions_.clear();
    elapsed_ms_ = 0;
}

}  // namespace realtime
