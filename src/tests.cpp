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

    // Rook: legal orthogonal move
    assert(runInput(" Board:\nwR . . .\n. . . .\nCommands:\nclick 50 50\nclick 350 50\nwait "
                    "1000\nprint board") == ". . . wR\n. . . .\n");
    // Rook: illegal diagonal move
    assert(runInput(" Board:\nwR . .\n. . .\nCommands:\nclick 50 50\nclick 150 150\nwait "
                    "1000\nprint board") == "wR . .\n. . .\n");
    // Rook: blocked by intermediate piece
    assert(runInput(" Board:\nwR wP . .\n. . . .\nCommands:\nclick 50 50\nclick 350 50\nwait "
                    "1000\nprint board") == "wR wP . .\n. . . .\n");

    // Bishop: legal diagonal move
    assert(runInput(" Board:\nwB . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 250 250\nwait "
                    "1000\nprint board") == ". . .\n. . .\n. . wB\n");
    // Bishop: illegal orthogonal move
    assert(runInput(" Board:\nwB . .\n. . .\nCommands:\nclick 50 50\nclick 250 50\nwait "
                    "1000\nprint board") == "wB . .\n. . .\n");

    // Queen: legal rook-like move
    assert(runInput(" Board:\nwQ . . .\n. . . .\nCommands:\nclick 50 50\nclick 350 50\nwait "
                    "1000\nprint board") == ". . . wQ\n. . . .\n");
    // Queen: illegal knight-like move
    assert(runInput(" Board:\nwQ . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 150 250\nwait "
                    "1000\nprint board") == "wQ . .\n. . .\n. . .\n");

    // Knight: legal L-shaped move
    assert(runInput(" Board:\nwN . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 150 250\nwait "
                    "1000\nprint board") == ". . .\n. . .\n. wN .\n");
    // Knight: illegal straight two-square move
    assert(runInput(" Board:\nwN . .\n. . .\nCommands:\nclick 50 50\nclick 250 50\nwait "
                    "1000\nprint board") == "wN . .\n. . .\n");

    return 0;
}
