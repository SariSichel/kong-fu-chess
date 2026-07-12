#pragma once

#include <iosfwd>
#include <string>

#include "../model/board.h"
#include "../model/piece.h"

namespace io {

class BoardPrinter {
public:
    static void print(const model::Board& board, std::ostream& out);
    static std::string pieceToToken(const model::Piece& piece);
    static char colorToChar(model::Color color);
    static char pieceTypeToChar(model::PieceType type);
};

}  // namespace io
