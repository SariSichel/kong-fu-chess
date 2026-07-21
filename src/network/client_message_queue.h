#pragma once

#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace network {

class ClientMessageQueue {
public:
    void push(const std::string& json);
    void drain(std::vector<std::string>& out);

private:
    std::mutex mutex_;
    std::queue<std::string> pending_;
};

}  // namespace network
