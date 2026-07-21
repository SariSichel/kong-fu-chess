#pragma once

#include <chrono>
#include <string>

namespace view {

struct DisconnectOverlayState {
    bool active = false;
    std::string disconnected_username;
    int seconds_remaining = 0;
    std::chrono::steady_clock::time_point grace_until{};

    void start(const std::string& username, int grace_seconds);
    void clear();
    void updateCountdown(std::chrono::steady_clock::time_point now);
    void applyMessage(const std::string& json);
};

}  // namespace view
