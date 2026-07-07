#include "board.h"

#include <sstream>

bool Board::isValidToken(const std::string& token) {
    if (token == ".") {
        return true;
    }
    if (token.length() == 2) {
        const std::string colors = "wb";
        const std::string pieces = "KQRBNP";
        return colors.find(token[0]) != std::string::npos &&
               pieces.find(token[1]) != std::string::npos;
    }
    return false;
}

ParseResult Board::parseFromInput(std::istream& in, Board& board) {
    std::string line;
    bool isParsingBoard = false;
    board.grid_.clear();

    while (std::getline(in, line)) {
        if (line == "Board:") {
            isParsingBoard = true;
            continue;
        }
        if (line == "Commands:") {
            isParsingBoard = false;
            break;
        }
        if (!isParsingBoard || line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> row;

        while (ss >> token) {
            if (!isValidToken(token)) {
                return ParseResult::ERROR_UNKNOWN_TOKEN;
            }
            row.push_back(token);
        }

        if (!row.empty()) {
            if (!board.grid_.empty() && row.size() != board.grid_[0].size()) {
                return ParseResult::ERROR_ROW_WIDTH_MISMATCH;
            }
            board.grid_.push_back(row);
        }
    }

    return ParseResult::OK;
}

void Board::print(std::ostream& out) const {
    for (size_t i = 0; i < grid_.size(); ++i) {
        for (size_t j = 0; j < grid_[i].size(); ++j) {
            out << grid_[i][j];
        }
    }
}

int Board::run(std::istream& in, std::ostream& out) {
    Board board;
    const ParseResult result = parseFromInput(in, board);

    if (result == ParseResult::ERROR_UNKNOWN_TOKEN) {
        out << "ERROR UNKNOWN_TOKEN" << std::endl;
        return 0;
    }
    if (result == ParseResult::ERROR_ROW_WIDTH_MISMATCH) {
        out << "ERROR ROW_WIDTH_MISMATCH" << std::endl;
        return 0;
    }

    board.print(out);
    return 0;
}
