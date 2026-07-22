#include "client_registry.h"

namespace network {

void ClientRegistry::registerClient(std::uint64_t connection_id, ix::WebSocket* web_socket) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_[connection_id] = web_socket;
}

void ClientRegistry::removeClient(std::uint64_t connection_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.erase(connection_id);
}

void ClientRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.clear();
}

void ClientRegistry::sendToConnection(std::uint64_t connection_id, const std::string& json) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = clients_.find(connection_id);
    if (it != clients_.end() && it->second != nullptr) {
        it->second->sendText(json);
    }
}

void ClientRegistry::broadcast(const std::string& json) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : clients_) {
        entry.second->sendText(json);
    }
}

}  // namespace network
