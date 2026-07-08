#ifndef BOARD_SERIALIZER_H
#define BOARD_SERIALIZER_H

#include <iosfwd>
#include <string>

class Board;

enum class ParseResult {
    OK,
    ERROR_UNKNOWN_TOKEN,
    ERROR_ROW_WIDTH_MISMATCH
};

class BoardSerializer {
public:
    static std::string trimLine(const std::string& line);
    static ParseResult parseFromInput(std::istream& in, Board& board);
    static void print(const Board& board, std::ostream& out);
};

#endif
