#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <iosfwd>
#include <vector>

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
        long finishAt;
    };

    static const char kFriendlyColor;

    void clearSelection();
    void requestMove(int fromR, int fromC, int toR, int toC);
    void processCompletedMoves(Board& board);

    long elapsedMs_ = 0;
    int selectedRow_ = -1;
    int selectedCol_ = -1;
    std::vector<PendingMove> pendingMoves_;
};

#endif
