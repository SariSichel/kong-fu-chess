#include "session_manager.h"

namespace session {

SessionManager::SessionManager(int port) : port_(port) {}

SessionManager::SessionManager(std::string white_username, std::string black_username, int port)
    : phase_(SessionPhase::InGame),
      fixed_roster_(true),
      white_username_(std::move(white_username)),
      black_username_(std::move(black_username)),
      port_(port) {}

std::optional<model::Color> SessionManager::expectedColorForUsername(
    const std::string& username) const {
    if (username == white_username_) {
        return model::Color::White;
    }
    if (username == black_username_) {
        return model::Color::Black;
    }
    return std::nullopt;
}

AuthResult SessionManager::authenticateLobby(std::uint64_t connection_id,
                                             const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (username.empty()) {
        return AuthResult::UnknownUser;
    }

    const auto existing_username = connection_usernames_.find(connection_id);
    if (existing_username != connection_usernames_.end()) {
        if (existing_username->second == username) {
            return AuthResult::Accepted;
        }
        return AuthResult::AlreadyConnected;
    }

    const auto username_it = username_connections_.find(username);
    if (username_it != username_connections_.end()) {
        return AuthResult::AlreadyConnected;
    }

    connection_usernames_[connection_id] = username;
    username_connections_[username] = connection_id;
    return AuthResult::Accepted;
}

AuthResult SessionManager::authenticate(std::uint64_t connection_id,
                                        const std::string& username) {
    if (!fixed_roster_) {
        return authenticateLobby(connection_id, username);
    }

    std::lock_guard<std::mutex> lock(mutex_);

    const std::optional<model::Color> expected_color = expectedColorForUsername(username);
    if (!expected_color.has_value()) {
        return AuthResult::UnknownUser;
    }

    const auto existing_connection = connection_colors_.find(connection_id);
    if (existing_connection != connection_colors_.end()) {
        const auto username_it = username_connections_.find(username);
        if (username_it != username_connections_.end() &&
            username_it->second == connection_id) {
            return AuthResult::Accepted;
        }
        return AuthResult::AlreadyConnected;
    }

    const auto username_it = username_connections_.find(username);
    if (username_it != username_connections_.end()) {
        return AuthResult::AlreadyConnected;
    }

    if (connection_colors_.size() >= kMaxPlayers) {
        return AuthResult::SlotFull;
    }

    connection_colors_[connection_id] = *expected_color;
    connection_usernames_[connection_id] = username;
    username_connections_[username] = connection_id;
    return AuthResult::Accepted;
}

void SessionManager::beginMatch(const matchmaking::MatchResult& match) {
    std::lock_guard<std::mutex> lock(mutex_);

    white_username_ = match.white.username;
    black_username_ = match.black.username;
    phase_ = SessionPhase::InGame;
    fixed_roster_ = true;

    connection_colors_[match.white.connection_id] = model::Color::White;
    connection_colors_[match.black.connection_id] = model::Color::Black;
    connection_usernames_[match.white.connection_id] = match.white.username;
    connection_usernames_[match.black.connection_id] = match.black.username;
    username_connections_[match.white.username] = match.white.connection_id;
    username_connections_[match.black.username] = match.black.connection_id;
}

void SessionManager::disconnect(std::uint64_t connection_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto username_it = connection_usernames_.find(connection_id);
    if (username_it != connection_usernames_.end()) {
        username_connections_.erase(username_it->second);
        connection_usernames_.erase(username_it);
    }

    connection_colors_.erase(connection_id);
}

std::optional<model::Color> SessionManager::colorFor(std::uint64_t connection_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto it = connection_colors_.find(connection_id);
    if (it == connection_colors_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<model::Color> SessionManager::colorForUsername(
    const std::string& username) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return expectedColorForUsername(username);
}

std::optional<std::string> SessionManager::usernameFor(std::uint64_t connection_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto it = connection_usernames_.find(connection_id);
    if (it == connection_usernames_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool SessionManager::isReady() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connection_colors_.size() == kMaxPlayers;
}

events::GameStarted SessionManager::rosterSnapshot() const {
    return events::GameStarted{white_username_, black_username_, port_};
}

}  // namespace session
