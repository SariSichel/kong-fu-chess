#include "disconnect_overlay.h"

#include "../network/ws_protocol.h"

#include <algorithm>
#include <chrono>

namespace view {

void DisconnectOverlayState::start(const std::string& username, int grace_seconds) {
    active = true;
    disconnected_username = username;
    seconds_remaining = grace_seconds;
    grace_until = std::chrono::steady_clock::now() + std::chrono::seconds(grace_seconds);
}

void DisconnectOverlayState::clear() {
    active = false;
    disconnected_username.clear();
    seconds_remaining = 0;
}

void DisconnectOverlayState::updateCountdown(std::chrono::steady_clock::time_point now) {
    if (!active) {
        return;
    }

    const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(grace_until - now);
    seconds_remaining = static_cast<int>(std::max<int64_t>(0, remaining.count()));

    if (seconds_remaining == 0) {
        active = false;
    }
}

void DisconnectOverlayState::applyMessage(const std::string& json) {
    switch (network::parseServerMessageType(json)) {
        case network::ServerMessageType::PlayerDisconnected: {
            const std::optional<network::PlayerDisconnectedMessage> message =
                network::parsePlayerDisconnectedMessage(json);
            if (message.has_value()) {
                start(message->username, message->grace_seconds);
            }
            break;
        }
        case network::ServerMessageType::PlayerReconnected:
            clear();
            break;
        case network::ServerMessageType::GameEnded:
            clear();
            break;
        default:
            break;
    }
}

}  // namespace view
