#pragma once

#include <iosfwd>
#include <string>

#include "../model/board.h"
#include "../model/piece.h"

namespace io {

enum class ParseResult {
    OK,
    ERROR_UNKNOWN_TOKEN,
    ERROR_ROW_WIDTH_MISMATCH
};

class BoardParser {
public:
    static std::string trimLine(const std::string& line);
    static ParseResult parseFromInput(std::istream& in, model::Board& board);
    static bool isValidToken(const std::string& token);
    static model::Color parseColor(char colorChar);
    static model::PieceType parsePieceType(char typeChar);
    static model::Piece parseToken(const std::string& token);
};

}  // namespace io
