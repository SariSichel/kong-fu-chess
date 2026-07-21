#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <string>

#include "match_info.h"

namespace network {
class LocalWsClient;
}  // namespace network

namespace session {

enum class LobbyStatus {
    Idle,
    Searching,
    Matched,
    Error,
};

struct LobbyButtonRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool contains(int px, int py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

struct LobbyState {
    LobbyStatus status = LobbyStatus::Idle;
    std::string username;
    int elo = 1200;
    std::string status_text = "Press Play to find a match";
    LobbyButtonRect play_button;
    bool play_enabled = true;
};

class ShellLobby {
public:
    ShellLobby(std::string username, int elo);

    std::optional<MatchInfo> run(network::LocalWsClient& client);

    void handleServerMessage(const std::string& json);
    void handleMouseClick(int x, int y, network::LocalWsClient& client);

private:
    void drainPendingMessages();
    void applyServerMessage(const std::string& json);
    void onPlayClicked(network::LocalWsClient& client);
    void drawFrame() const;
    bool containsPlayButton(int x, int y) const;

    LobbyState state_;
    std::optional<MatchInfo> pending_match_;
    bool exit_with_match_ = false;
    std::mutex messages_mutex_;
    std::queue<std::string> pending_messages_;
};

}  // namespace session
