#pragma once

#include <chrono>
#include <cstdint>

#include "../../events/event_bus.h"
#include "../../online/matchmaking/matchmaking_queue.h"
#include "../../online/disconnect_manager.h"
#include "../../online/session_manager.h"
#include "client_registry.h"

namespace network {

class QueueHandler;

class DisconnectHandler {
public:
    DisconnectHandler(session::SessionManager& session,
                      session::DisconnectManager& disconnect_manager,
                      QueueHandler& queue_handler, ClientRegistry& clients,
                      events::EventBus& bus);

    void handleDisconnect(std::uint64_t connection_id);
    void tickGrace(std::chrono::steady_clock::time_point now);

private:
    session::SessionManager& session_;
    session::DisconnectManager& disconnect_manager_;
    QueueHandler& queue_handler_;
    ClientRegistry& clients_;
    events::EventBus& bus_;
};

}  // namespace network
