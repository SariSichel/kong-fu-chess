#include "board.h"

#include <sstream>

#include "command_processor.h"

std::string Board::trimLine(const std::string& line) {
    const size_t start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const size_t end = line.find_last_not_of(" \t\r\n");
    return line.substr(start, end - start + 1);
}

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

bool Board::isEmpty(const std::string& token) {
    return token == ".";
}

bool Board::isFriendly(const std::string& token, char friendlyColor) {
    return !isEmpty(token) && token[0] == friendlyColor;
}

bool Board::inBounds(int row, int col) const {
    return row >= 0 && col >= 0 && static_cast<size_t>(row) < rows() &&
           static_cast<size_t>(col) < cols();
}

const std::string& Board::cell(int row, int col) const {
    return grid_[row][col];
}

void Board::movePiece(int fromR, int fromC, int toR, int toC) {
    grid_[toR][toC] = grid_[fromR][fromC];
    grid_[fromR][fromC] = ".";
}

ParseResult Board::parseFromInput(std::istream& in, Board& board) {
    std::string line;
    bool isParsingBoard = false;
    board.grid_.clear();

    while (std::getline(in, line)) {
        line = trimLine(line);

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
            if (j > 0) {
                out << ' ';
            }
            out << grid_[i][j];
        }
        out << '\n';
    }
}

int Board::run(std::istream& in, std::ostream& out) {
    return CommandProcessor::run(in, out);
}
