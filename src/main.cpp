//git link:
//https://github.com/SariSichel/kong-fu-chess/tree/main

#include "engine/game_engine.h"
#include "input/controller.h"
#include "io/board_setup.h"
#include "view/renderer.h"

// Test-only path — keep for reference, do NOT use in GUI main:
// #include "texttests/script_runner.h"

namespace {

constexpr const char* kBoardImagePath =
    R"(C:\Users\saris\OneDrive\Documents\bootcamp\CTD26\board.png)";

void runMouseLoopPlaceholder(engine::GameEngine& /*gameEngine*/,
                             input::Controller& /*controller*/,
                             view::Renderer& /*renderer*/) {
    // TODO: OpenCV event loop
    //   cv::setMouseCallback(...) → controller.handleClick(gameEngine, x, y)
    //   re-render after each change
    //   cv::waitKey(1) each frame
}

}  // namespace

int main() {
    engine::GameEngine gameEngine;
    input::Controller controller;
    view::Renderer renderer;

    io::setupStandardBoard(gameEngine.board());
    gameEngine.reset();

    renderer.render(gameEngine.board(), kBoardImagePath);

    // After show() returns (user pressed a key), replace with:
    // runMouseLoopPlaceholder(gameEngine, controller, renderer);

    return 0;
}
