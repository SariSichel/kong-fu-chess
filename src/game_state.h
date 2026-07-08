#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <iosfwd>
#include <vector>

#include "constants.h"

class Board;

class GameState {
public:
    void reset();

    void handleClick(Board& board, int x, int y);
    void advanceTime(long ms, Board& board);
    void printBoard(Board& board, std::ostream& out);

    long elapsedMs() const { return elapsedMs_; }
    bool hasSelection() const { return selectedRow_ >= 0; }
    int selectedRow() const { return selectedRow_; }
    int selectedCol() const { return selectedCol_; }

private:
    struct PendingMove {
        int fromR;
        int fromC;
        int toR;
        int toC;
        long startedAt;
        long finishAt;
    };

    void clearSelection();
    bool requestMove(int fromR, int fromC, int toR, int toC, Board& board);
    bool hasArrived(const PendingMove& move) const;
    void arriveAtDestination(Board& board, const PendingMove& move);
    void processCompletedMoves(Board& board);

    long elapsedMs_ = 0;
    int selectedRow_ = GameConfig::kNoSelection;
    int selectedCol_ = GameConfig::kNoSelection;
    std::vector<PendingMove> pendingMoves_;
};

#endif
