#pragma once

#include <string>

#include "../model/board.h"

class Img;

namespace view {

class Renderer {
public:
    void render(const model::Board& board, const std::string& boardImagePath);

private:
    void drawPieces(Img& canvas, const model::Board& board);
};

}  // namespace view
