#include <cassert>
#include <sstream>
#include <string>

#include "board.h"
#include "board_serializer.h"

namespace {

ParseResult parseInput(const std::string& input, Board& board) {
    std::istringstream in(input);
    return BoardSerializer::parseFromInput(in, board);
}

std::string runInput(const std::string& input) {
    std::istringstream in(input);
    std::ostringstream out;
    Board::run(in, out);
    return out.str();
}

}  // namespace

int main() {
    Board board;

    assert(parseInput(" Board:\nwP . wK\nwP . wP\nCommands:", board) == ParseResult::OK);
    assert(board.rows() == 2);
    assert(board.cols() == 3);

    {
        std::ostringstream out;
        BoardSerializer::print(board, out);
        assert(out.str() == "wP . wK\nwP . wP\n");
    }

    assert(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 150 150\nwait "
                    "1000\nprint board") == ". . .\n. wK .\n. . .\n");
    assert(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 150 150\nclick 250 250\nwait "
                    "1000\nprint board") == "wK . .\n. . .\n. . .\n");
    assert(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 350 50\nclick -10 50\nprint "
                    "board") == "wK . .\n. . .\n. . .\n");
    assert(runInput(" Board:\nwR . wK\n. . .\nCommands:\nclick 50 50\nclick 250 50\nclick 250 "
                    "150\nwait 1000\nprint board") == "wR . .\n. . wK\n");
    assert(runInput(" Board:\nwK xZ\n. .\nCommands:\nprint board") == "ERROR UNKNOWN_TOKEN");
    assert(runInput(" Board:\nwK . .\n. bK\nCommands:\nprint board") ==
           "ERROR ROW_WIDTH_MISMATCH");

    assert(parseInput("Board:\ninvalid\nCommands:", board) == ParseResult::ERROR_UNKNOWN_TOKEN);
    assert(parseInput("Board:\nwP .\nwP . wP\nCommands:", board) ==
           ParseResult::ERROR_ROW_WIDTH_MISMATCH);

    // King: legal one-square move
    assert(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 150 50\nwait "
                    "1000\nprint board") == ". wK .\n. . .\n. . .\n");
    // King: illegal two-square move
    assert(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 250 50\nwait "
                    "1000\nprint board") == "wK . .\n. . .\n. . .\n");

    // Rook: legal orthogonal move (3 cells — needs 3000ms)
    assert(runInput(" Board:\nwR . . .\n. . . .\nCommands:\nclick 50 50\nclick 350 50\nwait "
                    "3000\nprint board") == ". . . wR\n. . . .\n");
    // Rook: illegal diagonal move
    assert(runInput(" Board:\nwR . .\n. . .\nCommands:\nclick 50 50\nclick 150 150\nwait "
                    "1000\nprint board") == "wR . .\n. . .\n");
    // Rook: blocked by intermediate piece
    assert(runInput(" Board:\nwR wP . .\n. . . .\nCommands:\nclick 50 50\nclick 350 50\nwait "
                    "1000\nprint board") == "wR wP . .\n. . . .\n");

    // Bishop: legal diagonal move (2 cells — needs 2000ms)
    assert(runInput(" Board:\nwB . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 250 250\nwait "
                    "2000\nprint board") == ". . .\n. . .\n. . wB\n");
    // Bishop: illegal orthogonal move
    assert(runInput(" Board:\nwB . .\n. . .\nCommands:\nclick 50 50\nclick 250 50\nwait "
                    "1000\nprint board") == "wB . .\n. . .\n");

    // Queen: legal rook-like move (3 cells — needs 3000ms)
    assert(runInput(" Board:\nwQ . . .\n. . . .\nCommands:\nclick 50 50\nclick 350 50\nwait "
                    "3000\nprint board") == ". . . wQ\n. . . .\n");
    // Queen: illegal knight-like move
    assert(runInput(" Board:\nwQ . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 150 250\nwait "
                    "1000\nprint board") == "wQ . .\n. . .\n. . .\n");

    // Knight: legal L-shaped move (chebyshev distance 2 — needs 2000ms)
    assert(runInput(" Board:\nwN . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 150 250\nwait "
                    "2000\nprint board") == ". . .\n. . .\n. wN .\n");
    // Knight: illegal straight two-square move
    assert(runInput(" Board:\nwN . .\n. . .\nCommands:\nclick 50 50\nclick 250 50\nwait "
                    "1000\nprint board") == "wN . .\n. . .\n");
    // Knight: jumps over blocker on the way
    assert(runInput(" Board:\nwN . wP .\n. . . .\n. . . .\nCommands:\nclick 50 50\nclick 150 "
                    "250\nwait 2000\nprint board") == ". . wP .\n. . . .\n. wN . .\n");

    // Bishop: blocked by intermediate piece
    assert(runInput(" Board:\nwB . .\n. wP .\n. . .\nCommands:\nclick 50 50\nclick 250 250\nwait "
                    "1000\nprint board") == "wB . .\n. wP .\n. . .\n");

    // Rook: captures enemy on destination (3 cells — needs 3000ms)
    assert(runInput(" Board:\nwR . . bP\nCommands:\nclick 50 50\nclick 350 50\nwait 3000\nprint "
                    "board") == ". . . wR\n");
    // Rook: cannot move onto friendly piece
    assert(runInput(" Board:\nwR . . wP\nCommands:\nclick 50 50\nclick 350 50\nwait 1000\nprint "
                    "board") == "wR . . wP\n");
    // Knight: captures enemy on destination (chebyshev distance 2 — needs 2000ms)
    assert(runInput(" Board:\nwN . .\n. . bP\n. . .\nCommands:\nclick 50 50\nclick 250 150\nwait "
                    "2000\nprint board") == ". . .\n. . wN\n. . .\n");

    // Pawn: White moves 1 cell forward into empty square
    assert(runInput(" Board:\n. . .\nwP . .\n. . .\nCommands:\nclick 50 150\nclick 50 50\nwait "
                    "1000\nprint board") == "wP . .\n. . .\n. . .\n");
    // Pawn: White cannot move 2 cells forward
    assert(runInput(" Board:\n. . .\n. . .\nwP . .\nCommands:\nclick 50 250\nclick 50 50\nwait "
                    "1000\nprint board") == ". . .\n. . .\nwP . .\n");
    // Pawn: White cannot capture directly forward
    assert(runInput(" Board:\nbP . .\nwP . .\n. . .\nCommands:\nclick 50 150\nclick 50 50\nwait "
                    "1000\nprint board") == "bP . .\nwP . .\n. . .\n");
    // Pawn: White cannot move diagonally to empty square
    assert(runInput(" Board:\n. . .\nwP . .\n. . .\nCommands:\nclick 50 150\nclick 150 50\nwait "
                    "1000\nprint board") == ". . .\nwP . .\n. . .\n");
    // Pawn: White captures enemy diagonally
    assert(runInput(" Board:\n. bP .\nwP . .\n. . .\nCommands:\nclick 50 150\nclick 150 50\nwait "
                    "1000\nprint board") == ". wP .\n. . .\n. . .\n");

    // Rook: 2-cell move — still at source before arrival, at destination after
    assert(runInput(" Board:\nwR . .\nCommands:\nclick 50 50\nclick 250 50\nwait 1000\nprint "
                    "board\nwait 1000\nprint board") == "wR . .\n. . wR\n");
    // Bishop: 2-cell diagonal — still at source before arrival, at destination after
    assert(runInput(" Board:\nwB . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 250 250\nwait 1000\n"
                    "print board\nwait 1000\nprint board") ==
           "wB . .\n. . .\n. . .\n. . .\n. . .\n. . wB\n");

    // --- Architecture-review gap tests (TDD — expected to fail until logic is fixed) ---

    // Gap 1: first selection must ignore enemy pieces (only friendly/white may be selected).
    // Current bug: handleClick selects any non-empty cell on the first click, so bK moves.
    assert(runInput(" Board:\nbK . wK\n. . .\nCommands:\nclick 50 50\nclick 150 150\nwait "
                    "1000\nprint board") == "bK . wK\n. . .\n");

    // Gap 2: two pending moves with independent finish times must complete separately.
    // Rook (0,0)->(0,2) needs 2000ms; king (1,0)->(1,1) needs 1000ms.
    // After 1000ms only the king arrives; rook stays at source until 2000ms total.
    assert(runInput(" Board:\nwR . .\nwK . .\nCommands:\nclick 50 50\nclick 250 50\nclick 50 "
                    "150\nclick 150 150\nwait 1000\nprint board\nwait 1000\nprint board") ==
           "wR . .\n. wK .\n. . wR\n. wK .\n");

    // Gap 3: print immediately after queuing a move (no wait) must show the settled board
    // with the piece still at its source — in-flight moves must not appear at destination.
    assert(runInput(" Board:\nwR . .\nCommands:\nclick 50 50\nclick 250 50\nprint board") ==
           "wR . .\n");

    return 0;
}
