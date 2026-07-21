#include "disconnect_manager.h"

namespace session {

DisconnectManager::DisconnectManager(int grace_ms) : grace_ms_(grace_ms) {}

void DisconnectManager::onDisconnect(const DisconnectGraceEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    grace_entries_[entry.username] = entry;
}

bool DisconnectManager::onReconnect(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    return grace_entries_.erase(username) > 0;
}

bool DisconnectManager::isInGrace(const std::string& username) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return grace_entries_.find(username) != grace_entries_.end();
}

std::optional<DisconnectGraceEntry> DisconnectManager::graceEntryFor(
    const std::string& username) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto it = grace_entries_.find(username);
    if (it == grace_entries_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::optional<model::Color> DisconnectManager::tick(std::chrono::steady_clock::time_point now) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = grace_entries_.begin(); it != grace_entries_.end(); ++it) {
        if (now >= it->second.grace_until) {
            const model::Color winner =
                it->second.color == model::Color::White ? model::Color::Black : model::Color::White;
            grace_entries_.erase(it);
            return winner;
        }
    }

    return std::nullopt;
}

}  // namespace session
