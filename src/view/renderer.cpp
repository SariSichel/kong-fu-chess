#include "renderer.h"

#include <opencv2/highgui.hpp>
#include <string>

#include "../constants.h"
#include "../model/piece.h"
#include "../model/position.h"
#include "piece_sprite.h"
#include "render_helpers.h"

namespace view {

void Renderer::render(const model::Board& board, const std::string& boardImagePath) {
    cv::Mat canvas = render::loadBoardImage(boardImagePath);

    drawPieces(canvas, board);

    cv::imshow("Image", canvas);
    cv::waitKey(0);
    cv::destroyAllWindows();
}

void Renderer::drawPieces(cv::Mat& canvas, const model::Board& board) {
    const int cellSize = GameConfig::kClickCellSize;
    const int originX = GameConfig::kBoardOriginX;
    const int originY = GameConfig::kBoardOriginY;

    for (size_t row = 0; row < board.rows(); ++row) {
        for (size_t col = 0; col < board.cols(); ++col) {
            const model::Piece& piece = board.cell(model::Position{static_cast<int>(row),
                                                                   static_cast<int>(col)});
            if (piece.isEmpty()) {
                continue;
            }

            const cv::Mat sprite =
                render::loadSpriteResized(idleSpritePath(pieceToSpriteCode(piece)), cellSize,
                                          cellSize);

            const int cellLeft = originX + static_cast<int>(col) * cellSize;
            const int cellTop = originY + static_cast<int>(row) * cellSize;
            const int x = render::centeredDrawCoord(cellLeft, cellSize, sprite.cols);
            const int y = render::centeredDrawCoord(cellTop, cellSize, sprite.rows);

            render::blitSpriteWithAlpha(canvas, sprite, x, y);
        }
    }
}

}  // namespace view
