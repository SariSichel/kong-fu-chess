#include "client_message_queue.h"

namespace network {

void ClientMessageQueue::push(const std::string& json) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.push(json);
}

void ClientMessageQueue::drain(std::vector<std::string>& out) {
    std::queue<std::string> pending;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending.swap(pending_);
    }

    out.clear();
    while (!pending.empty()) {
        out.push_back(std::move(pending.front()));
        pending.pop();
    }
}

}  // namespace network
