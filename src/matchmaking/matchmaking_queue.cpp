#include "matchmaking_queue.h"

#include <algorithm>
#include <cstdlib>

namespace matchmaking {

MatchmakingQueue::MatchmakingQueue(int elo_match_range) : elo_match_range_(elo_match_range) {}

bool MatchmakingQueue::isEligible(const QueueEntry& a, const QueueEntry& b) const {
    return std::abs(a.elo - b.elo) <= elo_match_range_;
}

std::optional<MatchResult> MatchmakingQueue::enqueue(QueueEntry entry) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = queue_.begin(); it != queue_.end(); ++it) {
        if (!isEligible(*it, entry)) {
            continue;
        }

        MatchResult result{*it, entry};
        queue_.erase(it);
        return result;
    }

    queue_.push_back(std::move(entry));
    return std::nullopt;
}

bool MatchmakingQueue::remove(std::uint64_t connection_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto it = std::find_if(queue_.begin(), queue_.end(),
                                 [connection_id](const QueueEntry& entry) {
                                     return entry.connection_id == connection_id;
                                 });
    if (it == queue_.end()) {
        return false;
    }

    queue_.erase(it);
    return true;
}

bool MatchmakingQueue::contains(std::uint64_t connection_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::any_of(queue_.begin(), queue_.end(), [connection_id](const QueueEntry& entry) {
        return entry.connection_id == connection_id;
    });
}

}  // namespace matchmaking
