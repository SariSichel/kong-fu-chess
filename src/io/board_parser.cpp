#include "board_parser.h"

#include <sstream>
#include <vector>

#include "../config/game_config.h"

namespace io {

bool BoardParser::isValidToken(const std::string& token) {
    if (token.length() == 1 && token[0] == BoardTokens::kEmpty) {
        return true;
    }
    if (token.length() == 2) {
        return std::string(BoardTokens::kColors).find(token[0]) != std::string::npos &&
               std::string(BoardTokens::kPieceTypes).find(token[1]) != std::string::npos;
    }
    return false;
}

model::Color BoardParser::parseColor(char colorChar) {
    return colorChar == 'w' ? model::Color::White : model::Color::Black;
}

model::PieceType BoardParser::parsePieceType(char typeChar) {
    switch (typeChar) {
        case BoardTokens::kKing:
            return model::PieceType::King;
        case BoardTokens::kQueen:
            return model::PieceType::Queen;
        case BoardTokens::kRook:
            return model::PieceType::Rook;
        case BoardTokens::kBishop:
            return model::PieceType::Bishop;
        case BoardTokens::kKnight:
            return model::PieceType::Knight;
        case BoardTokens::kPawn:
            return model::PieceType::Pawn;
        default:
            return model::PieceType::Empty;
    }
}

model::Piece BoardParser::parseToken(const std::string& token) {
    if (token.length() == 1 && token[0] == BoardTokens::kEmpty) {
        return model::Piece::empty();
    }
    return model::Piece(parsePieceType(token[1]), parseColor(token[0]));
}

std::string BoardParser::trimLine(const std::string& line) {
    const size_t start = line.find_first_not_of(Text::kWhitespace);
    if (start == std::string::npos) {
        return "";
    }
    const size_t end = line.find_last_not_of(Text::kWhitespace);
    return line.substr(start, end - start + 1);
}

ParseResult BoardParser::parseFromInput(std::istream& in, model::Board& board) {
    std::string line;
    bool isParsingBoard = false;
    board.clear();

    while (std::getline(in, line)) {
        line = trimLine(line);

        if (line == InputMarkers::kBoardSection) {
            isParsingBoard = true;
            continue;
        }
        if (line == InputMarkers::kCommandsSection) {
            isParsingBoard = false;
            break;
        }
        if (!isParsingBoard || line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string token;
        std::vector<model::Piece> row;

        while (ss >> token) {
            if (!isValidToken(token)) {
                return ParseResult::ERROR_UNKNOWN_TOKEN;
            }
            row.push_back(parseToken(token));
        }

        if (!row.empty()) {
            if (board.rows() > 0 && row.size() != board.cols()) {
                return ParseResult::ERROR_ROW_WIDTH_MISMATCH;
            }
            board.addRow(std::move(row));
        }
    }

    return ParseResult::OK;
}

}  // namespace io
