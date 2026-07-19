#include "command_queue.h"

namespace network {

void CommandQueue::push(QueuedCommand command) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.push_back(std::move(command));
}

void CommandQueue::drain(std::vector<QueuedCommand>& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    out.assign(pending_.begin(), pending_.end());
    pending_.clear();
}

}  // namespace network
