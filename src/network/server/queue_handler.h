#pragma once

#include <ixwebsocket/IXWebSocket.h>

#include <chrono>
#include <cstdint>

#include "../../db/user_store.h"
#include "../../events/event_bus.h"
#include "../../online/matchmaking/matchmaking_queue.h"
#include "../../online/session_manager.h"
#include "client_registry.h"

namespace network {

class QueueHandler {
public:
    QueueHandler(session::SessionManager& session, db::UserStore& user_store,
                 matchmaking::MatchmakingQueue& matchmaking_queue, ClientRegistry& clients,
                 events::EventBus& bus, int port);

    void handleJoinQueue(std::uint64_t connection_id, ix::WebSocket& web_socket);
    void handleLeaveQueue(std::uint64_t connection_id, ix::WebSocket& web_socket);
    void removeFromQueue(std::uint64_t connection_id);
    void tickQueueTimeouts(std::chrono::steady_clock::time_point now);

private:
    void notifyMatch(const matchmaking::MatchResult& match);

    session::SessionManager& session_;
    db::UserStore& user_store_;
    matchmaking::MatchmakingQueue& matchmaking_queue_;
    ClientRegistry& clients_;
    events::EventBus& bus_;
    int port_;
};

}  // namespace network
