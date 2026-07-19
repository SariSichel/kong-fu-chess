//git link:
//https://github.com/SariSichel/kong-fu-chess/tree/main

#include "engine/game_engine.h"
#include "input/controller.h"
#include "io/board_setup.h"
#include "view/renderer.h"

#include "constants.h"
#include <opencv2/highgui.hpp>

// Test-only path — keep for reference, do NOT use in GUI main:
// #include "texttests/script_runner.h"

namespace {

constexpr int kFrameMs = 16;

struct AppState {
    engine::GameEngine* gameEngine = nullptr;
    input::Controller* controller = nullptr;
    bool running = true;
};

void onMouse(int event, int x, int y, int /*flags*/, void* userdata) {
    auto* state = static_cast<AppState*>(userdata);
    if (event == cv::EVENT_LBUTTONDOWN) {
        state->controller->handleClick(*state->gameEngine, x, y);
    } else if (event == cv::EVENT_RBUTTONDOWN) {
        state->controller->handleJump(*state->gameEngine, x, y);
    }
}

void runGameLoop(engine::GameEngine& gameEngine, input::Controller& controller,
                 view::Renderer& renderer) {
    AppState state{&gameEngine, &controller, true};

    renderer.init(AssetPaths::kBoardImage);
    cv::namedWindow(view::Renderer::kWindowName);
    cv::setMouseCallback(view::Renderer::kWindowName, onMouse, &state);

    while (state.running) {
        gameEngine.advanceTime(kFrameMs);
        renderer.drawFrame(gameEngine, controller);

        const int key = cv::waitKey(kFrameMs);
        if (key == 27) {
            state.running = false;
        }
    }

    cv::destroyAllWindows();
}

}  // namespace

int main() {
    engine::GameEngine gameEngine;
    input::Controller controller;
    view::Renderer renderer;

    io::setupStandardBoard(gameEngine.board());
    gameEngine.reset();

    runGameLoop(gameEngine, controller, renderer);

    return 0;
}
