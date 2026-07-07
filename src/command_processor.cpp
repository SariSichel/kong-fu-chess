#include "command_processor.h"

#include <sstream>

#include "board.h"
#include "game_state.h"

namespace CommandProcessor {

void processLine(const std::string& line, std::ostream& out, Board& board, GameState& state) {
    const std::string trimmed = Board::trimLine(line);
    if (trimmed.empty()) {
        return;
    }

    std::stringstream ss(trimmed);
    std::string cmd;
    ss >> cmd;

    if (cmd == "click") {
        int x = 0;
        int y = 0;
        if (ss >> x >> y) {
            state.handleClick(board, x, y);
        }
    } else if (cmd == "wait") {
        long ms = 0;
        if (ss >> ms) {
            state.advanceTime(ms, board);
        }
    } else if (cmd == "print") {
        std::string subcmd;
        if (ss >> subcmd && subcmd == "board") {
            state.printBoard(board, out);
            out.flush();
        }
    }
}

void processCommands(std::istream& in, std::ostream& out, Board& board, GameState& state) {
    std::string line;

    while (std::getline(in, line)) {
        processLine(line, out, board, state);
    }
}

int run(std::istream& in, std::ostream& out) {
    Board board;
    GameState state;

    const ParseResult result = Board::parseFromInput(in, board);
    if (result == ParseResult::ERROR_UNKNOWN_TOKEN) {
        out << "ERROR UNKNOWN_TOKEN";
        out.flush();
        return 0;
    }
    if (result == ParseResult::ERROR_ROW_WIDTH_MISMATCH) {
        out << "ERROR ROW_WIDTH_MISMATCH";
        out.flush();
        return 0;
    }

    state.reset();
    processCommands(in, out, board, state);
    return 0;
}

}  // namespace CommandProcessor
