#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "ws_protocol.h"

namespace network {

struct QueuedCommand {
    std::uint64_t connection_id = 0;
    ClientCommand command;
};

class CommandQueue {
public:
    void push(QueuedCommand command);
    void drain(std::vector<QueuedCommand>& out);

private:
    mutable std::mutex mutex_;
    std::deque<QueuedCommand> pending_;
};

}  // namespace network
