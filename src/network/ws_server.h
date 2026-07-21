#pragma once

#include <memory>

#include "../db/user_store.h"
#include "../events/event_bus.h"
#include "../matchmaking/matchmaking_queue.h"
#include "../session/session_manager.h"
#include "command_queue.h"

namespace network {

class WsServer {
public:
    WsServer(session::SessionManager& session, events::EventBus& bus, db::UserStore& user_store,
             CommandQueue& command_queue, int port);
    ~WsServer();

    WsServer(const WsServer&) = delete;
    WsServer& operator=(const WsServer&) = delete;

    bool start();
    void stop();
    void tick();
    int port() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace network
