#include "shell_lobby.h"

#include "../constants.h"
#include "../network/local_ws_client.h"
#include "../network/ws_protocol.h"
#include "../view/render_helpers.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <sstream>

namespace session {

namespace {

constexpr const char* kWindowName = "Kong Fu Chess - Lobby";

struct LobbyMouseContext {
    ShellLobby* lobby = nullptr;
    network::LocalWsClient* client = nullptr;
};

void onLobbyMouse(int event, int x, int y, int /*flags*/, void* userdata) {
    auto* context = static_cast<LobbyMouseContext*>(userdata);
    if (context == nullptr || context->lobby == nullptr || context->client == nullptr) {
        return;
    }

    if (event == cv::EVENT_LBUTTONDOWN) {
        context->lobby->handleMouseClick(x, y, *context->client);
    }
}

}  // namespace

ShellLobby::ShellLobby(std::string username, int elo) {
    state_.username = std::move(username);
    state_.elo = elo;
    state_.play_button = {LobbyConfig::kPlayButtonX, LobbyConfig::kPlayButtonY,
                          LobbyConfig::kPlayButtonWidth, LobbyConfig::kPlayButtonHeight};
}

int ShellLobby::run(network::LocalWsClient& client) {
    cv::namedWindow(kWindowName);
    cv::moveWindow(kWindowName, 200, 120);

    LobbyMouseContext mouse_context{this, &client};
    cv::setMouseCallback(kWindowName, onLobbyMouse, &mouse_context);

    while (true) {
        drainPendingMessages();
        drawFrame();

        const int key = cv::waitKey(LobbyConfig::kFrameMs);
        if (key == 27) {
            break;
        }
    }

    cv::destroyWindow(kWindowName);
    return 0;
}

void ShellLobby::handleServerMessage(const std::string& json) {
    std::lock_guard<std::mutex> lock(messages_mutex_);
    pending_messages_.push(json);
}

void ShellLobby::drainPendingMessages() {
    std::queue<std::string> messages;
    {
        std::lock_guard<std::mutex> lock(messages_mutex_);
        messages.swap(pending_messages_);
    }

    while (!messages.empty()) {
        applyServerMessage(messages.front());
        messages.pop();
    }
}

void ShellLobby::applyServerMessage(const std::string& json) {
    switch (network::parseServerMessageType(json)) {
        case network::ServerMessageType::QueueJoined:
            state_.status = LobbyStatus::Searching;
            state_.status_text = "Searching for opponent...";
            state_.play_enabled = false;
            break;
        case network::ServerMessageType::QueueLeft:
            state_.status = LobbyStatus::Idle;
            state_.status_text = "Press Play to find a match";
            state_.play_enabled = true;
            break;
        case network::ServerMessageType::Error:
            state_.status = LobbyStatus::Error;
            state_.status_text = "Failed to join queue";
            state_.play_enabled = true;
            break;
        default:
            break;
    }
}

void ShellLobby::handleMouseClick(int x, int y, network::LocalWsClient& client) {
    if (containsPlayButton(x, y)) {
        onPlayClicked(client);
    }
}

void ShellLobby::onPlayClicked(network::LocalWsClient& client) {
    if (!state_.play_enabled || state_.status == LobbyStatus::Searching) {
        return;
    }

    state_.status = LobbyStatus::Searching;
    state_.status_text = "Searching for opponent...";
    state_.play_enabled = false;

    if (!client.sendJoinQueue()) {
        state_.status = LobbyStatus::Error;
        state_.status_text = "Failed to join queue";
        state_.play_enabled = true;
    }
}

bool ShellLobby::containsPlayButton(int x, int y) const {
    return state_.play_enabled && state_.play_button.contains(x, y);
}

void ShellLobby::drawFrame() const {
    cv::Mat canvas(LobbyConfig::kWindowHeight, LobbyConfig::kWindowWidth, CV_8UC3,
                   cv::Scalar(36, 36, 36));

    view::render::drawTextLine(canvas, "Kong Fu Chess", 24, 40, 1.0, cv::Scalar(240, 240, 240),
                               2);

    view::render::drawTextLine(canvas, "Player: " + state_.username, 24, 90, 0.7,
                               cv::Scalar(220, 220, 220));

    std::ostringstream elo_line;
    elo_line << "ELO: " << state_.elo;
    view::render::drawTextLine(canvas, elo_line.str(), 24, 130, 0.7, cv::Scalar(220, 220, 220));

    const cv::Scalar status_color =
        state_.status == LobbyStatus::Error ? cv::Scalar(80, 80, 255) : cv::Scalar(180, 220, 180);
    view::render::drawTextLine(canvas, state_.status_text, 24, 180, 0.6, status_color);

    const cv::Scalar button_fill =
        state_.play_enabled ? cv::Scalar(60, 140, 60) : cv::Scalar(80, 80, 80);
    const cv::Scalar button_border =
        state_.play_enabled ? cv::Scalar(90, 200, 90) : cv::Scalar(110, 110, 110);

    const cv::Rect play_rect(state_.play_button.x, state_.play_button.y, state_.play_button.width,
                             state_.play_button.height);
    cv::rectangle(canvas, play_rect, button_fill, cv::FILLED);
    cv::rectangle(canvas, play_rect, button_border, 2);

    const cv::Scalar label_color =
        state_.play_enabled ? cv::Scalar(255, 255, 255) : cv::Scalar(180, 180, 180);
    view::render::drawTextLine(canvas, "Play", state_.play_button.x + 72,
                               state_.play_button.y + 34, 0.9, label_color, 2);

    cv::imshow(kWindowName, canvas);
}

}  // namespace session
