#include <cassert>
#include <sstream>
#include <string>

#include "board.h"

namespace {

ParseResult parseInput(const std::string& input, Board& board) {
    std::istringstream in(input);
    return Board::parseFromInput(in, board);
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

    assert(parseInput("Board:\nwP . wK\nwP . wP\nCommands:", board) == ParseResult::OK);
    assert(board.rows() == 2);
    assert(board.cols() == 3);
    assert(board.grid()[0][0] == "wP");
    assert(board.grid()[0][1] == ".");
    assert(board.grid()[1][2] == "wP");

    {
        std::ostringstream out;
        board.print(out);
        assert(out.str() == "wP.wKwP.wP");
    }

    assert(parseInput("Board:\ninvalid\nCommands:", board) == ParseResult::ERROR_UNKNOWN_TOKEN);
    assert(parseInput("Board:\nwP .\nwP . wP\nCommands:", board) ==
           ParseResult::ERROR_ROW_WIDTH_MISMATCH);

    assert(parseInput("Board:\n.\nCommands:", board) == ParseResult::OK);
    assert(board.rows() == 1);
    assert(board.cols() == 1);

    assert(parseInput("Board:\nCommands:", board) == ParseResult::OK);
    assert(board.rows() == 0);

    assert(parseInput("ignored\nBoard:\nbK wQ\nCommands:", board) == ParseResult::OK);
    assert(board.rows() == 1);
    assert(board.cols() == 2);

    assert(runInput("Board:\nwP . wK\nwP . wP\nCommands:") == "wP.wKwP.wP");
    assert(runInput("Board:\nxx\nCommands:") == "ERROR UNKNOWN_TOKEN\n");
    assert(runInput("Board:\nwP .\nwP . wP\nCommands:") == "ERROR ROW_WIDTH_MISMATCH\n");

    for (const std::string& token : {".", "wK", "bQ", "wR", "bB", "wN", "bP"}) {
        assert(parseInput("Board:\n" + token + "\nCommands:", board) == ParseResult::OK);
    }

    assert(parseInput("Board:\nwX\nCommands:", board) == ParseResult::ERROR_UNKNOWN_TOKEN);
    assert(parseInput("Board:\nK\nCommands:", board) == ParseResult::ERROR_UNKNOWN_TOKEN);
    assert(parseInput("Board:\nwPP\nCommands:", board) == ParseResult::ERROR_UNKNOWN_TOKEN);

    return 0;
}
