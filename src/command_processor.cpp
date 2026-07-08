#include "command_processor.h"

#include <sstream>

#include "board.h"
#include "constants.h"
#include "game_state.h"

namespace CommandProcessor {

namespace {

const char* parseErrorMessage(ParseResult result) {
    switch (result) {
        case ParseResult::ERROR_UNKNOWN_TOKEN:
            return ErrorMessages::kUnknownToken;
        case ParseResult::ERROR_ROW_WIDTH_MISMATCH:
            return ErrorMessages::kRowWidthMismatch;
        default:
            return nullptr;
    }
}

}  // namespace

void processLine(const std::string& line, std::ostream& out, Board& board, GameState& state) {
    const std::string trimmed = Board::trimLine(line);
    if (trimmed.empty()) {
        return;
    }

    std::stringstream ss(trimmed);
    std::string cmd;
    ss >> cmd;

    if (cmd == Commands::kClick) {
        int x = 0;
        int y = 0;
        if (ss >> x >> y) {
            state.handleClick(board, x, y);
        }
    } else if (cmd == Commands::kWait) {
        long ms = 0;
        if (ss >> ms) {
            state.advanceTime(ms, board);
        }
    } else if (cmd == Commands::kPrint) {
        std::string subcmd;
        if (ss >> subcmd && subcmd == Commands::kPrintBoard) {
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
    if (result != ParseResult::OK) {
        if (const char* message = parseErrorMessage(result)) {
            out << message;
            out.flush();
        }
        return 0;
    }

    state.reset();
    processCommands(in, out, board, state);
    return 0;
}

}  // namespace CommandProcessor
