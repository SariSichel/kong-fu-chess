#include "renderer.h"

#include "img.hpp"

namespace view {

void Renderer::render(const model::Board& board, const std::string& boardImagePath) {
    Img canvas;
    canvas.read(boardImagePath);

    drawPieces(board);

    canvas.show();
}

void Renderer::drawPieces(const model::Board& board) {
    (void)board;
    // TODO: iterate board.cell() and draw CTD26 sprites with draw_on()
}

}  // namespace view
