#include "board_printer.h"

#include <ostream>

#include "../config/game_config.h"
#include "../model/position.h"

namespace io {

char BoardPrinter::colorToChar(model::Color color) {
    return color == model::Color::White ? 'w' : 'b';
}

char BoardPrinter::pieceTypeToChar(model::PieceType type) {
    switch (type) {
        case model::PieceType::King:
            return BoardTokens::kKing;
        case model::PieceType::Queen:
            return BoardTokens::kQueen;
        case model::PieceType::Rook:
            return BoardTokens::kRook;
        case model::PieceType::Bishop:
            return BoardTokens::kBishop;
        case model::PieceType::Knight:
            return BoardTokens::kKnight;
        case model::PieceType::Pawn:
            return BoardTokens::kPawn;
        case model::PieceType::Empty:
        default:
            return BoardTokens::kEmpty;
    }
}

std::string BoardPrinter::pieceToToken(const model::Piece& piece) {
    if (piece.isEmpty()) {
        return std::string(1, BoardTokens::kEmpty);
    }
    return std::string({colorToChar(piece.color()), pieceTypeToChar(piece.type())});
}

void BoardPrinter::print(const model::Board& board, std::ostream& out) {
    for (size_t row = 0; row < board.rows(); ++row) {
        for (size_t col = 0; col < board.cols(); ++col) {
            if (col > 0) {
                out << ' ';
            }
            out << pieceToToken(board.cell(model::Position{static_cast<int>(row), static_cast<int>(col)}));
        }
        out << '\n';
    }
}

}  // namespace io
