#include "game_state.h"

#include <algorithm>
#include <cstdlib>
#include <utility>

#include "board.h"
#include "board_serializer.h"
#include "constants.h"
#include "piece.h"

namespace {

long moveDurationMs(int fromR, int fromC, int toR, int toC) {
    const int dr = std::abs(toR - fromR);
    const int dc = std::abs(toC - fromC);
    const int distance = std::max(dr, dc);
    return static_cast<long>(distance) * GameConfig::kMoveDurationMs;
}

bool squareArrivedThisTick(const std::vector<std::pair<int, int>>& arrivedThisTick, int row,
                           int col) {
    return std::any_of(arrivedThisTick.begin(), arrivedThisTick.end(),
                       [row, col](const std::pair<int, int>& square) {
                           return square.first == row && square.second == col;
                       });
}

}  // namespace

void GameState::reset() {
    elapsedMs_ = 0;
    selectedRow_ = GameConfig::kNoSelection;
    selectedCol_ = GameConfig::kNoSelection;
    pendingMoves_.clear();
}

void GameState::clearSelection() {
    selectedRow_ = GameConfig::kNoSelection;
    selectedCol_ = GameConfig::kNoSelection;
}

void GameState::handleClick(Board& board, int x, int y) {
    if (x < 0 || y < 0) {
        return;
    }

    const int col = x / GameConfig::kClickCellSize;
    const int row = y / GameConfig::kClickCellSize;

    if (!board.inBounds(row, col)) {
        return;
    }

    const Piece& piece = board.cell(row, col);

    if (!hasSelection()) {
        if (piece.isEmpty() || !piece.isFriendly(GameConfig::kFriendlyColor)) {
            return;
        }
        selectedRow_ = row;
        selectedCol_ = col;
        return;
    }

    if (!piece.isEmpty() && piece.isFriendly(GameConfig::kFriendlyColor)) {
        selectedRow_ = row;
        selectedCol_ = col;
        return;
    }

    if (!board.canMove(selectedRow_, selectedCol_, row, col)) {
        return;
    }

    if (!requestMove(selectedRow_, selectedCol_, row, col, board)) {
        return;
    }

    clearSelection();
}

bool GameState::requestMove(int fromR, int fromC, int toR, int toC, Board& board) {
    Piece& piece = board.cell(fromR, fromC);
    if (piece.isMoving()) {
        return false;
    }

    const long durationMs = moveDurationMs(fromR, fromC, toR, toC);
    piece.beginMove(fromR, fromC, toR, toC);
    pendingMoves_.push_back(
        {fromR, fromC, toR, toC, elapsedMs_, elapsedMs_ + durationMs});
    return true;
}

bool GameState::hasArrived(const PendingMove& move) const {
    const long durationMs = move.finishAt - move.startedAt;
    if (durationMs <= 0) {
        return true;
    }

    const long elapsedOnMove = elapsedMs_ - move.startedAt;
    return elapsedOnMove >= durationMs;
}

bool GameState::pendingMoveLess(const PendingMove& a, const PendingMove& b) {
    if (a.startedAt != b.startedAt) {
        return a.startedAt < b.startedAt;
    }
    if (a.fromR != b.fromR) {
        return a.fromR < b.fromR;
    }
    return a.fromC < b.fromC;
}

void GameState::resolveArrival(Board& board, const PendingMove& move,
                              std::vector<std::pair<int, int>>& arrivedThisTick) {
    if (!board.inBounds(move.fromR, move.fromC)) {
        return;
    }

    Piece& mover = board.cell(move.fromR, move.fromC);
    if (mover.isEmpty() || !mover.isMoving()) {
        return;
    }

    const Piece& destination = board.cell(move.toR, move.toC);
    if (!destination.isEmpty()) {
        if (destination.isFriendly(mover.color())) {
            board.cancelMoveAt(move.fromR, move.fromC);
            return;
        }

        if (squareArrivedThisTick(arrivedThisTick, move.toR, move.toC)) {
            board.removePieceAt(move.fromR, move.fromC);
            return;
        }
    }

    board.arrivePiece(move.fromR, move.fromC, move.toR, move.toC);
    arrivedThisTick.push_back({move.toR, move.toC});
}

void GameState::processCompletedMoves(Board& board) {
    std::vector<PendingMove> completingMoves;
    completingMoves.reserve(pendingMoves_.size());

    for (const PendingMove& move : pendingMoves_) {
        if (hasArrived(move)) {
            completingMoves.push_back(move);
        }
    }

    if (completingMoves.empty()) {
        return;
    }

    pendingMoves_.erase(std::remove_if(pendingMoves_.begin(), pendingMoves_.end(),
                                       [this](const PendingMove& move) {
                                           return hasArrived(move);
                                       }),
                        pendingMoves_.end());

    std::sort(completingMoves.begin(), completingMoves.end(), pendingMoveLess);

    std::vector<std::pair<int, int>> arrivedThisTick;
    arrivedThisTick.reserve(completingMoves.size());

    for (const PendingMove& move : completingMoves) {
        resolveArrival(board, move, arrivedThisTick);
    }
}

void GameState::advanceTime(long ms, Board& board) {
    elapsedMs_ += ms;
    processCompletedMoves(board);
}

void GameState::printBoard(Board& board, std::ostream& out) {
    processCompletedMoves(board);
    BoardSerializer::print(board, out);
}
