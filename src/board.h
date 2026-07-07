#ifndef BOARD_H
#define BOARD_H

#include <iostream>
#include <string>
#include <vector>

enum class ParseResult {
    OK,
    ERROR_UNKNOWN_TOKEN,
    ERROR_ROW_WIDTH_MISMATCH
};

class Board {
public:
    Board() = default;

    static int run(std::istream& in, std::ostream& out);
    static ParseResult parseFromInput(std::istream& in, Board& board);
    static std::string trimLine(const std::string& line);

    void print(std::ostream& out) const;

    const std::vector<std::vector<std::string>>& grid() const { return grid_; }
    size_t rows() const { return grid_.size(); }
    size_t cols() const { return grid_.empty() ? 0 : grid_[0].size(); }

    bool inBounds(int row, int col) const;
    const std::string& cell(int row, int col) const;
    void movePiece(int fromR, int fromC, int toR, int toC);

    static bool isEmpty(const std::string& token);
    static bool isFriendly(const std::string& token, char friendlyColor);

private:
    static bool isValidToken(const std::string& token);

    std::vector<std::vector<std::string>> grid_;
};

#endif
