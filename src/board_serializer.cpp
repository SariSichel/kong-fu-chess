#include "board_serializer.h"

#include <sstream>
#include <vector>

#include "board.h"
#include "constants.h"
#include "piece.h"

namespace {

bool isValidToken(const std::string& token) {
    if (token.length() == 1 && token[0] == BoardTokens::kEmpty) {
        return true;
    }
    if (token.length() == 2) {
        return std::string(BoardTokens::kColors).find(token[0]) != std::string::npos &&
               std::string(BoardTokens::kPieceTypes).find(token[1]) != std::string::npos;
    }
    return false;
}

Color parseColor(char colorChar) {
    return colorChar == 'w' ? Color::White : Color::Black;
}

PieceType parsePieceType(char typeChar) {
    switch (typeChar) {
        case BoardTokens::kKing:
            return PieceType::King;
        case BoardTokens::kQueen:
            return PieceType::Queen;
        case BoardTokens::kRook:
            return PieceType::Rook;
        case BoardTokens::kBishop:
            return PieceType::Bishop;
        case BoardTokens::kKnight:
            return PieceType::Knight;
        case BoardTokens::kPawn:
            return PieceType::Pawn;
        default:
            return PieceType::Empty;
    }
}

Piece parseToken(const std::string& token) {
    if (token.length() == 1 && token[0] == BoardTokens::kEmpty) {
        return Piece::empty();
    }
    return Piece(parsePieceType(token[1]), parseColor(token[0]));
}

char colorToChar(Color color) {
    return color == Color::White ? 'w' : 'b';
}

char pieceTypeToChar(PieceType type) {
    switch (type) {
        case PieceType::King:
            return BoardTokens::kKing;
        case PieceType::Queen:
            return BoardTokens::kQueen;
        case PieceType::Rook:
            return BoardTokens::kRook;
        case PieceType::Bishop:
            return BoardTokens::kBishop;
        case PieceType::Knight:
            return BoardTokens::kKnight;
        case PieceType::Pawn:
            return BoardTokens::kPawn;
        case PieceType::Empty:
        default:
            return BoardTokens::kEmpty;
    }
}

std::string pieceToToken(const Piece& piece) {
    if (piece.isEmpty()) {
        return std::string(1, BoardTokens::kEmpty);
    }
    return std::string({colorToChar(piece.color()), pieceTypeToChar(piece.type())});
}

}  // namespace

std::string BoardSerializer::trimLine(const std::string& line) {
    const size_t start = line.find_first_not_of(Text::kWhitespace);
    if (start == std::string::npos) {
        return "";
    }
    const size_t end = line.find_last_not_of(Text::kWhitespace);
    return line.substr(start, end - start + 1);
}

ParseResult BoardSerializer::parseFromInput(std::istream& in, Board& board) {
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
        std::vector<Piece> row;

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

void BoardSerializer::print(const Board& board, std::ostream& out) {
    for (size_t row = 0; row < board.rows(); ++row) {
        for (size_t col = 0; col < board.cols(); ++col) {
            if (col > 0) {
                out << ' ';
            }
            out << pieceToToken(board.cell(static_cast<int>(row), static_cast<int>(col)));
        }
        out << '\n';
    }
}
