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

// A pawn that has just settled on the furthest-forward row for its color is
// immediately promoted to a Queen. White advances toward row 0; Black advances
// toward the last row.
void promotePawnIfEligible(Board& board, int row, int col) {
    Piece& piece = board.cell(row, col);
    if (piece.type() != PieceType::Pawn) {
        return;
    }
    const int lastRow =
        (piece.color() == Color::White) ? 0 : static_cast<int>(board.rows()) - 1;
    if (row == lastRow) {
        piece.promote(PieceType::Queen);
    }
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
    nextJumpId_ = 0;
    selectedRow_ = GameConfig::kNoSelection;
    selectedCol_ = GameConfig::kNoSelection;
    pendingMoves_.clear();
    pendingJumps_.clear();
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

void GameState::handleJump(Board& board, int x, int y) {
    // The jump command uses the same pixel-coordinate convention as click.
    if (x < 0 || y < 0) {
        return;
    }

    const int col = x / GameConfig::kClickCellSize;
    const int row = y / GameConfig::kClickCellSize;

    // Initiating a jump clears any pending selection.
    if (requestJump(row, col, board)) {
        clearSelection();
    }
}

bool GameState::requestMove(int fromR, int fromC, int toR, int toC, Board& board) {
    if (gameOver_) {
        return false;
    }

    Piece& piece = board.cell(fromR, fromC);
    // Both moving and airborne pieces are "busy" and cannot start a new move.
    if (piece.isBusy()) {
        return false;
    }

    const long durationMs = moveDurationMs(fromR, fromC, toR, toC);
    piece.beginMove(fromR, fromC, toR, toC);
    pendingMoves_.push_back(
        {fromR, fromC, toR, toC, elapsedMs_, elapsedMs_ + durationMs, nextMoveId_++});
    return true;
}

bool GameState::requestJump(int row, int col, Board& board) {
    if (gameOver_) {
        return false;
    }

    if (!board.inBounds(row, col)) {
        return false;
    }

    Piece& piece = board.cell(row, col);
    // canJump() rejects empty (captured) pieces and anything not currently idle
    // (already moving or already airborne).
    if (!piece.canJump()) {
        return false;
    }

    piece.beginJump(row, col);
    pendingJumps_.push_back(
        {row, col, elapsedMs_, elapsedMs_ + GameConfig::kJumpDurationMs, nextJumpId_++});
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
            // Includes the case where the ally on the square is airborne: a
            // friendly move already in flight is canceled and the mover stays put.
            board.cancelMoveAt(move.fromR, move.fromC);
            clearPremoveAt(move.fromR, move.fromC);
            return;
        }

        // An airborne enemy captures the arriving piece: the mover is destroyed,
        // the airborne piece stays on its cell, and its jump keeps running until
        // the timer expires (handled by processCompletedJumps).
        if (destination.isAirborne()) {
            const bool moverIsKing = mover.type() == PieceType::King;
            const Color defenderColor = destination.color();
            board.removePieceAt(move.fromR, move.fromC);
            clearPremoveAt(move.fromR, move.fromC);
            if (moverIsKing) {
                gameOver_ = true;
                winner_ = defenderColor;
            }
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

    promotePawnIfEligible(board, move.toR, move.toC);

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

void GameState::processCompletedJumps(Board& board) {
    if (pendingJumps_.empty()) {
        return;
    }

    std::vector<PendingJump> completed;
    completed.reserve(pendingJumps_.size());
    for (const PendingJump& jump : pendingJumps_) {
        if (elapsedMs_ >= jump.finishAt) {
            completed.push_back(jump);
        }
    }

    if (completed.empty()) {
        return;
    }

    pendingJumps_.erase(std::remove_if(pendingJumps_.begin(), pendingJumps_.end(),
                                       [this](const PendingJump& jump) {
                                           return elapsedMs_ >= jump.finishAt;
                                       }),
                        pendingJumps_.end());

    for (const PendingJump& jump : completed) {
        if (!board.inBounds(jump.row, jump.col)) {
            continue;
        }
        Piece& piece = board.cell(jump.row, jump.col);
        if (piece.isAirborne()) {
            piece.finishJump();
        }
    }
}

void GameState::advanceTime(long ms, Board& board) {
    elapsedMs_ += ms;
    // Resolve arrivals before landing jumps so that an enemy arriving on the exact
    // tick a jump ends is still met by an airborne defender (inclusive window: the
    // piece is airborne for the full [start, finish] duration, including finish).
    processCompletedMoves(board);
    processCompletedJumps(board);
}

void GameState::printBoard(Board& board, std::ostream& out) {
    processCompletedMoves(board);
    processCompletedJumps(board);
    BoardSerializer::print(board, out);
}
