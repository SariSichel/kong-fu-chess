#pragma once

#include <cstdint>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "../events/game_event.h"
#include "../model/piece.h"

namespace session {

inline constexpr std::size_t kMaxPlayers = 2;

enum class AuthResult {
    Accepted,
    UnknownUser,
    SlotFull,
    AlreadyConnected,
};

class SessionManager {
public:
    SessionManager(std::string white_username, std::string black_username, int port);

    AuthResult authenticate(std::uint64_t connection_id, const std::string& username);
    void disconnect(std::uint64_t connection_id);

    std::optional<model::Color> colorFor(std::uint64_t connection_id) const;
    std::optional<model::Color> colorForUsername(const std::string& username) const;

    bool isReady() const;
    events::GameStarted rosterSnapshot() const;

    const std::string& whiteUsername() const { return white_username_; }
    const std::string& blackUsername() const { return black_username_; }

private:
    std::optional<model::Color> expectedColorForUsername(const std::string& username) const;

    std::string white_username_;
    std::string black_username_;
    int port_;

    mutable std::mutex mutex_;
    std::unordered_map<std::uint64_t, model::Color> connection_colors_;
    std::unordered_map<std::string, std::uint64_t> username_connections_;
};

}  // namespace session
