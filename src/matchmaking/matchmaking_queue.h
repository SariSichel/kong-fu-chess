#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace matchmaking {

struct QueueEntry {
    std::uint64_t connection_id = 0;
    std::string username;
    int elo = 1200;
    std::chrono::steady_clock::time_point enqueued_at{};
};

struct MatchResult {
    QueueEntry white;
    QueueEntry black;
};

class MatchmakingQueue {
public:
    explicit MatchmakingQueue(int elo_match_range = 100);

    std::optional<MatchResult> enqueue(QueueEntry entry);
    bool remove(std::uint64_t connection_id);
    bool contains(std::uint64_t connection_id) const;

private:
    bool isEligible(const QueueEntry& a, const QueueEntry& b) const;

    int elo_match_range_;
    mutable std::mutex mutex_;
    std::vector<QueueEntry> queue_;
};

}  // namespace matchmaking
