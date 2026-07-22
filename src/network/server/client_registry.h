#pragma once

#include <ixwebsocket/IXWebSocket.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace network {

class ClientRegistry {
public:
    void registerClient(std::uint64_t connection_id, ix::WebSocket* web_socket);
    void removeClient(std::uint64_t connection_id);
    void clear();

    void sendToConnection(std::uint64_t connection_id, const std::string& json);
    void broadcast(const std::string& json);

private:
    std::mutex mutex_;
    std::unordered_map<std::uint64_t, ix::WebSocket*> clients_;
};

}  // namespace network
