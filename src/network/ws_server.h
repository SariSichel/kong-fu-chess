#pragma once

#include <memory>

#include "../events/event_bus.h"
#include "../session/session_manager.h"
#include "command_queue.h"

namespace network {

class WsServer {
public:
    WsServer(session::SessionManager& session, events::EventBus& bus, CommandQueue& command_queue,
             int port);
    ~WsServer();

    WsServer(const WsServer&) = delete;
    WsServer& operator=(const WsServer&) = delete;

    bool start();
    void stop();
    int port() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace network
