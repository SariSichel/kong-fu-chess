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

bool GameState::isActiveMoveStartTick(int fromR, int fromC) const {
    return std::any_of(pendingMoves_.begin(), pendingMoves_.end(),
                       [fromR, fromC, this](const PendingMove& move) {
                           return move.fromR == fromR && move.fromC == fromC &&
                                  move.startedAt == elapsedMs_;
                       });
}

void GameState::reset() {
    elapsedMs_ = 0;
    nextMoveId_ = 0;
    selectedRow_ = GameConfig::kNoSelection;
    selectedCol_ = GameConfig::kNoSelection;
    pendingMoves_.clear();
    premoves_.clear();
    gameOver_ = false;
    winner_ = Color::White;
}

void GameState::clearSelection() {
    selectedRow_ = GameConfig::kNoSelection;
    selectedCol_ = GameConfig::kNoSelection;
}

bool GameState::hasPremoveAt(int sourceR, int sourceC) const {
    return premoves_.find({sourceR, sourceC}) != premoves_.end();
}

void GameState::queuePremove(int keyR, int keyC, int fromR, int fromC, int toR, int toC) {
    premoves_[{keyR, keyC}] = {fromR, fromC, toR, toC};
}

void GameState::clearPremoveAt(int sourceR, int sourceC) {
    premoves_.erase({sourceR, sourceC});
}

void GameState::handleClick(Board& board, int x, int y) {
    if (gameOver_) {
        return;
    }

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
        if (piece.isEmpty()) {
            return;
        }
        selectedRow_ = row;
        selectedCol_ = col;
        return;
    }

    if (!piece.isEmpty()) {
        const Piece& selectedPiece = board.cell(selectedRow_, selectedCol_);
        if (piece.color() == selectedPiece.color()) {
            selectedRow_ = row;
            selectedCol_ = col;
            return;
        }
    }

    Piece& selectedPiece = board.cell(selectedRow_, selectedCol_);
    if (selectedPiece.isMoving()) {
        if (isActiveMoveStartTick(selectedRow_, selectedCol_)) {
            queuePremove(selectedRow_, selectedCol_, selectedPiece.destinationRow(),
                         selectedPiece.destinationCol(), row, col);
        }
        clearSelection();
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
    if (gameOver_) {
        return false;
    }

    Piece& piece = board.cell(fromR, fromC);
    if (piece.isMoving()) {
        return false;
    }

    const long durationMs = moveDurationMs(fromR, fromC, toR, toC);
    piece.beginMove(fromR, fromC, toR, toC);
    pendingMoves_.push_back(
        {fromR, fromC, toR, toC, elapsedMs_, elapsedMs_ + durationMs, nextMoveId_++});
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
    return a.moveId < b.moveId;
}

void GameState::tryExecutePremove(Board& board, int moveSourceR, int moveSourceC, int arrivalR,
                                  int arrivalC) {
    const auto it = premoves_.find({moveSourceR, moveSourceC});
    if (it == premoves_.end()) {
        return;
    }

    const Premove premove = it->second;
    premoves_.erase(it);

    if (premove.fromR != arrivalR || premove.fromC != arrivalC) {
        return;
    }

    if (!board.canMove(arrivalR, arrivalC, premove.toR, premove.toC)) {
        return;
    }

    requestMove(arrivalR, arrivalC, premove.toR, premove.toC, board);
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
            clearPremoveAt(move.fromR, move.fromC);
            return;
        }

        if (squareArrivedThisTick(arrivedThisTick, move.toR, move.toC)) {
            board.removePieceAt(move.fromR, move.fromC);
            clearPremoveAt(move.fromR, move.fromC);
            return;
        }
    }

    const bool capturedKing = !destination.isEmpty() && destination.type() == PieceType::King &&
                              !destination.isFriendly(mover.color());
    if (capturedKing) {
        gameOver_ = true;
        winner_ = mover.color();
    }

    board.arrivePiece(move.fromR, move.fromC, move.toR, move.toC);
    arrivedThisTick.push_back({move.toR, move.toC});

    if (capturedKing) {
        return;
    }

    tryExecutePremove(board, move.fromR, move.fromC, move.toR, move.toC);
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
        if (gameOver_) {
            break;
        }
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
