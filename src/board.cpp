#include "board.h"

#include <cstdlib>
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

int Board::sign(int delta) {
    if (delta > 0) {
        return 1;
    }
    if (delta < 0) {
        return -1;
    }
    return 0;
}

bool Board::isPathClear(int fromR, int fromC, int toR, int toC) const {
    const int stepR = sign(toR - fromR);
    const int stepC = sign(toC - fromC);
    int r = fromR + stepR;
    int c = fromC + stepC;

    while (r != toR || c != toC) {
        if (!isEmpty(cell(r, c))) {
            return false;
        }
        r += stepR;
        c += stepC;
    }

    return true;
}

bool Board::canKingMove(int fromR, int fromC, int toR, int toC) const {
    const int dr = std::abs(toR - fromR);
    const int dc = std::abs(toC - fromC);
    return dr <= 1 && dc <= 1 && (dr != 0 || dc != 0);
}

bool Board::canRookMove(int fromR, int fromC, int toR, int toC) const {
    if (fromR != toR && fromC != toC) {
        return false;
    }
    return isPathClear(fromR, fromC, toR, toC);
}

bool Board::canBishopMove(int fromR, int fromC, int toR, int toC) const {
    const int dr = toR - fromR;
    const int dc = toC - fromC;
    if (std::abs(dr) != std::abs(dc)) {
        return false;
    }
    return isPathClear(fromR, fromC, toR, toC);
}

bool Board::canQueenMove(int fromR, int fromC, int toR, int toC) const {
    return canRookMove(fromR, fromC, toR, toC) ||
           canBishopMove(fromR, fromC, toR, toC);
}

bool Board::canKnightMove(int fromR, int fromC, int toR, int toC) const {
    const int dr = std::abs(toR - fromR);
    const int dc = std::abs(toC - fromC);
    return (dr == 1 && dc == 2) || (dr == 2 && dc == 1);
}

bool Board::canMove(int fromR, int fromC, int toR, int toC) const {
    if (fromR == toR && fromC == toC) {
        return false;
    }

    const std::string& piece = cell(fromR, fromC);
    if (isEmpty(piece)) {
        return false;
    }

    switch (piece[1]) {
        case 'K':
            return canKingMove(fromR, fromC, toR, toC);
        case 'R':
            return canRookMove(fromR, fromC, toR, toC);
        case 'B':
            return canBishopMove(fromR, fromC, toR, toC);
        case 'Q':
            return canQueenMove(fromR, fromC, toR, toC);
        case 'N':
            return canKnightMove(fromR, fromC, toR, toC);
        case 'P':
        default:
            return false;
    }
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
