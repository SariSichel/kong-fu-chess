#include "session_manager.h"

namespace session {

SessionManager::SessionManager(std::string white_username, std::string black_username, int port)
    : white_username_(std::move(white_username)),
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

AuthResult SessionManager::authenticate(std::uint64_t connection_id,
                                       const std::string& username) {
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
    username_connections_[username] = connection_id;
    return AuthResult::Accepted;
}

void SessionManager::disconnect(std::uint64_t connection_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto connection_it = connection_colors_.find(connection_id);
    if (connection_it == connection_colors_.end()) {
        return;
    }

    for (auto username_it = username_connections_.begin();
         username_it != username_connections_.end(); ++username_it) {
        if (username_it->second == connection_id) {
            username_connections_.erase(username_it);
            break;
        }
    }

    connection_colors_.erase(connection_it);
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
    return expectedColorForUsername(username);
}

bool SessionManager::isReady() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connection_colors_.size() == kMaxPlayers;
}

events::GameStarted SessionManager::rosterSnapshot() const {
    return events::GameStarted{white_username_, black_username_, port_};
}

}  // namespace session
