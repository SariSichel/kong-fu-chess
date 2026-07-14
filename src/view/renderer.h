#pragma once

#include <string>

#include "../model/board.h"

namespace cv {
class Mat;
}

namespace view {

class Renderer {
public:
    void render(const model::Board& board, const std::string& boardImagePath);

private:
    void drawPieces(cv::Mat& canvas, const model::Board& board);
};

}  // namespace view
