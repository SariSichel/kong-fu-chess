#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "../model/piece.h"

namespace session {

struct DisconnectGraceEntry {
    std::string username;
    model::Color color = model::Color::White;
    std::uint64_t connection_id = 0;
    std::chrono::steady_clock::time_point disconnected_at{};
    std::chrono::steady_clock::time_point grace_until{};
};

class DisconnectManager {
public:
    explicit DisconnectManager(int grace_ms = 20000);

    void onDisconnect(const DisconnectGraceEntry& entry);
    bool onReconnect(const std::string& username);
    bool isInGrace(const std::string& username) const;
    std::optional<DisconnectGraceEntry> graceEntryFor(const std::string& username) const;

    std::optional<model::Color> tick(std::chrono::steady_clock::time_point now);

private:
    int grace_ms_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, DisconnectGraceEntry> grace_entries_;
};

}  // namespace session
