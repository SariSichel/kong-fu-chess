#include "game_state.h"

#include "board.h"
#include "board_serializer.h"
#include "constants.h"

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
        if (piece.isEmpty()) {
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
    BoardSerializer::print(board, out);
}
