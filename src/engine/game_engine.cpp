#include "game_engine.h"

#include <algorithm>
#include <cstdlib>

#include "../config/game_config.h"
#include "../events/event_bus.h"
#include "../events/game_event.h"
#include "../rules/rule_engine.h"

namespace engine {

void GameEngine::reset() {
    arbiter_.clear();
    next_motion_id_ = 0;
    premoves_.clear();
    is_game_over_ = false;
    winner_ = std::nullopt;
    move_log_.clear();
}

void GameEngine::applyRemoteGameEnded(model::Color winner) {
    is_game_over_ = true;
    winner_ = winner;
}

int GameEngine::elapsedMs() const {
    return arbiter_.elapsedMs();
}

const std::vector<realtime::Motion>& GameEngine::activeMotions() const {
    return arbiter_.getActiveMotions();
}

namespace {

bool isJumpActiveAt(const realtime::RealTimeArbiter& arbiter, const model::Position& pos) {
    return std::any_of(arbiter.getActiveMotions().begin(), arbiter.getActiveMotions().end(),
                       [&](const realtime::Motion& motion) {
                           return motion.type == realtime::MotionType::Jump && motion.source == pos;
                       });
}

}  // namespace

void GameEngine::advanceTime(int ms) {
    const int futureElapsed = arbiter_.elapsedMs() + ms;
    std::vector<realtime::Motion> completingMoves;
    completingMoves.reserve(arbiter_.getActiveMotions().size());

    for (const realtime::Motion& motion : arbiter_.getActiveMotions()) {
        if (motion.type == realtime::MotionType::Move && motion.isExpired(futureElapsed)) {
            completingMoves.push_back(motion);
        }
    }

    checkGameOverFromCompletingMoves(completingMoves);
    const std::vector<realtime::MoveResolution> resolutions =
        arbiter_.advanceTime(ms, board_);
    processMoveResolutions(resolutions);
    processPremovesAfterAdvance(completingMoves);
    applyCooldownAfterArrivals(completingMoves);
}

void GameEngine::processMoveResolutions(
    const std::vector<realtime::MoveResolution>& resolutions) {
    for (const realtime::MoveResolution& resolution : resolutions) {
        if (resolution.kind == realtime::MoveResolution::Kind::CompletedMove) {
            move_log_.recordCompletedMove(resolution.completed);
            if (event_bus_ != nullptr) {
                event_bus_->publish(resolution.completed);
            }
        } else {
            move_log_.recordJumpCapture(resolution.jump_capture);
            if (event_bus_ != nullptr) {
                event_bus_->publish(resolution.jump_capture);
            }
        }
    }
}

bool GameEngine::requestMove(const model::Position& from, const model::Position& to) {
    if (is_game_over_) {
        return false;
    }

    if (isBusyAt(from)) {
        return false;
    }

    if (!rules::RuleEngine::validateMove(board_, from, to)) {
        return false;
    }

    const model::Piece& piece = board_.cell(from);
    const int dr = std::abs(to.row - from.row);
    const int dc = std::abs(to.col - from.col);
    const int distance = std::max(dr, dc);
    const int durationMs = distance * static_cast<int>(GameConfig::kMoveDurationMs);

    arbiter_.addMotion(realtime::Motion(static_cast<int>(next_motion_id_++), from, to,
                                        realtime::MotionType::Move, arbiter_.elapsedMs(),
                                        durationMs, piece.type(), piece.color()));

    if (event_bus_ != nullptr) {
        realtime::MoveStartedEvent started;
        started.timestamp_ms = arbiter_.elapsedMs();
        started.piece_type = piece.type();
        started.piece_color = piece.color();
        started.from = from;
        started.to = to;
        event_bus_->publish(started);
    }

    return true;
}

bool GameEngine::requestJump(const model::Position& pos) {
    if (is_game_over_) {
        return false;
    }

    if (!board_.inBounds(pos)) {
        return false;
    }

    const model::Piece& piece = board_.cell(pos);
    if (piece.isEmpty() || piece.jumpCooldownRemainingMs() > 0 || isBusyAt(pos)) {
        return false;
    }

    arbiter_.addMotion(realtime::Motion(static_cast<int>(next_motion_id_++), pos,
                                        model::Position(-1, -1), realtime::MotionType::Jump,
                                        arbiter_.elapsedMs(),
                                        static_cast<int>(GameConfig::kJumpDurationMs),
                                        piece.type(), piece.color()));

    if (event_bus_ != nullptr) {
        realtime::JumpStartedEvent started;
        started.timestamp_ms = arbiter_.elapsedMs();
        started.piece_type = piece.type();
        started.piece_color = piece.color();
        started.square = pos;
        event_bus_->publish(started);
    }

    return true;
}

bool GameEngine::hasPremoveAt(const model::Position& source) const {
    return premoves_.find(premoveKey(source)) != premoves_.end();
}

bool GameEngine::isActiveMoveStartTick(const model::Position& from) const {
    return std::any_of(arbiter_.getActiveMotions().begin(), arbiter_.getActiveMotions().end(),
                       [&](const realtime::Motion& motion) {
                           return motion.type == realtime::MotionType::Move && motion.source == from &&
                                  motion.started_at_ms == arbiter_.elapsedMs();
                       });
}

bool GameEngine::isBusyAt(const model::Position& pos) const {
    return std::any_of(arbiter_.getActiveMotions().begin(), arbiter_.getActiveMotions().end(),
                       [&](const realtime::Motion& motion) { return motion.source == pos; });
}

bool GameEngine::getActiveMoveDestination(const model::Position& from,
                                          model::Position& destination) const {
    for (const realtime::Motion& motion : arbiter_.getActiveMotions()) {
        if (motion.type == realtime::MotionType::Move && motion.source == from) {
            destination = motion.destination;
            return true;
        }
    }
    return false;
}

void GameEngine::queuePremove(const model::Position& key, const model::Position& from,
                              const model::Position& to) {
    premoves_[premoveKey(key)] = {from, to};
}

void GameEngine::clearPremoveAt(const model::Position& source) {
    premoves_.erase(premoveKey(source));
}

void GameEngine::tryExecutePremove(const model::Position& moveSource,
                                     const model::Position& arrival) {
    const auto it = premoves_.find(premoveKey(moveSource));
    if (it == premoves_.end()) {
        return;
    }

    const Premove premove = it->second;
    premoves_.erase(it);

    if (premove.from != arrival) {
        return;
    }

    if (!rules::RuleEngine::validateMove(board_, arrival, premove.to)) {
        return;
    }

    requestMove(arrival, premove.to);
}

void GameEngine::checkGameOverFromCompletingMoves(
    const std::vector<realtime::Motion>& completingMoves) {
    if (is_game_over_) {
        return;
    }

    const auto endGame = [this](model::Color winnerColor) {
        is_game_over_ = true;
        winner_ = winnerColor;
        if (event_bus_ != nullptr) {
            event_bus_->publish(events::GameEnded{winnerColor});
        }
    };

    for (const realtime::Motion& motion : completingMoves) {
        if (motion.type != realtime::MotionType::Move) {
            continue;
        }

        const model::Piece& mover = board_.cell(motion.source);
        if (mover.isEmpty()) {
            continue;
        }

        const model::Piece& destination = board_.cell(motion.destination);
        if (!destination.isEmpty() &&
            destination.type() == model::PieceType::King &&
            !destination.isFriendly(mover.color())) {
            endGame(mover.color());
            return;
        }

        if (isJumpActiveAt(arbiter_, motion.destination) && mover.type() == model::PieceType::King) {
            endGame(mover.color());
            return;
        }
    }
}

void GameEngine::processPremovesAfterAdvance(
    const std::vector<realtime::Motion>& completingMoves) {
    if (is_game_over_) {
        return;
    }

    for (const realtime::Motion& motion : completingMoves) {
        if (motion.type != realtime::MotionType::Move) {
            continue;
        }

        const model::Piece& sourceCell = board_.cell(motion.source);
        const model::Piece& destinationCell = board_.cell(motion.destination);

        if (!sourceCell.isEmpty()) {
            clearPremoveAt(motion.source);
            continue;
        }

        if (destinationCell.isEmpty()) {
            clearPremoveAt(motion.source);
            continue;
        }

        tryExecutePremove(motion.source, motion.destination);
    }
}

void GameEngine::applyCooldownAfterArrivals(
    const std::vector<realtime::Motion>& completingMoves) {
    for (const realtime::Motion& motion : completingMoves) {
        if (motion.type != realtime::MotionType::Move) {
            continue;
        }

        if (!board_.inBounds(motion.destination)) {
            continue;
        }

        const model::Piece& piece = board_.cell(motion.destination);
        if (piece.isEmpty()) {
            continue;
        }

        if (isBusyAt(motion.destination)) {
            continue;
        }

        board_.cell(motion.destination)
            .setMoveCooldown(static_cast<int>(GameConfig::kMoveCooldownMs));
    }
}

}  // namespace engine
