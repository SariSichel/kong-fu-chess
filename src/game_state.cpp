#include "game_state.h"

#include "board.h"

const char GameState::kFriendlyColor = 'w';

void GameState::reset() {
    elapsedMs_ = 0;
    selectedRow_ = -1;
    selectedCol_ = -1;
    pendingMoves_.clear();
}

void GameState::clearSelection() {
    selectedRow_ = -1;
    selectedCol_ = -1;
}

void GameState::handleClick(Board& board, int x, int y) {
    if (x < 0 || y < 0) {
        return;
    }

    const int col = x / 100;
    const int row = y / 100;

    if (!board.inBounds(row, col)) {
        return;
    }

    const std::string& cell = board.cell(row, col);

    if (!hasSelection()) {
        if (Board::isEmpty(cell)) {
            return;
        }
        selectedRow_ = row;
        selectedCol_ = col;
        return;
    }

    if (!Board::isEmpty(cell) && Board::isFriendly(cell, kFriendlyColor)) {
        selectedRow_ = row;
        selectedCol_ = col;
        return;
    }

    requestMove(selectedRow_, selectedCol_, row, col);
    clearSelection();
}

void GameState::requestMove(int fromR, int fromC, int toR, int toC) {
    pendingMoves_.push_back({fromR, fromC, toR, toC, elapsedMs_});
}

void GameState::processCompletedMoves(Board& board) {
    for (size_t i = 0; i < pendingMoves_.size();) {
        const PendingMove& move = pendingMoves_[i];
        if (elapsedMs_ < move.finishAt) {
            ++i;
            continue;
        }

        board.movePiece(move.fromR, move.fromC, move.toR, move.toC);
        pendingMoves_.erase(pendingMoves_.begin() + static_cast<long>(i));
    }
}

void GameState::advanceTime(long ms, Board& board) {
    elapsedMs_ += ms;
    processCompletedMoves(board);
}

void GameState::printBoard(Board& board, std::ostream& out) {
    processCompletedMoves(board);
    board.print(out);
}
