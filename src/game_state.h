#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <cstdint>
#include <iosfwd>
#include <map>
#include <utility>
#include <vector>

#include "constants.h"

class Board;

class GameState {
public:
    void reset();

    void handleClick(Board& board, int x, int y);
    void handleJump(Board& board, int x, int y);
    void advanceTime(std::int64_t ms, Board& board);
    void printBoard(Board& board, std::ostream& out);

    std::int64_t elapsedMs() const { return elapsedMs_; }
    bool hasSelection() const { return selectedRow_ >= 0; }
    int selectedRow() const { return selectedRow_; }
    int selectedCol() const { return selectedCol_; }

    bool hasPremoveAt(int sourceR, int sourceC) const;
    bool isGameOver() const { return gameOver_; }

private:
    struct Premove {
        int fromR;
        int fromC;
        int toR;
        int toC;
    };

    struct PendingMove {
        int fromR;
        int fromC;
        int toR;
        int toC;
        std::int64_t startedAt;
        std::int64_t finishAt;
        uint64_t moveId;
    };

    struct PendingJump {
        int row;
        int col;
        std::int64_t startedAt;
        std::int64_t finishAt;
        uint64_t jumpId;
    };

    void clearSelection();
    bool requestMove(int fromR, int fromC, int toR, int toC, Board& board);
    bool requestJump(int row, int col, Board& board);
    void queuePremove(int keyR, int keyC, int fromR, int fromC, int toR, int toC);
    void clearPremoveAt(int sourceR, int sourceC);
    void tryExecutePremove(Board& board, int moveSourceR, int moveSourceC, int arrivalR,
                           int arrivalC);
    bool isActiveMoveStartTick(int fromR, int fromC) const;
    bool hasArrived(const PendingMove& move) const;
    static bool pendingMoveLess(const PendingMove& a, const PendingMove& b);
    void resolveArrival(Board& board, const PendingMove& move,
                        std::vector<std::pair<int, int>>& arrivedThisTick);
    void processCompletedMoves(Board& board);
    void processCompletedJumps(Board& board);

    std::int64_t elapsedMs_ = 0;
    uint64_t nextMoveId_ = 0;
    uint64_t nextJumpId_ = 0;
    int selectedRow_ = GameConfig::kNoSelection;
    int selectedCol_ = GameConfig::kNoSelection;
    std::vector<PendingMove> pendingMoves_;
    std::vector<PendingJump> pendingJumps_;
    std::map<std::pair<int, int>, Premove> premoves_;
    bool gameOver_ = false;
};

#endif
