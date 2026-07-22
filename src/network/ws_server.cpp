#include "ws_server.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include <chrono>
#include <memory>

#include "../config/network_config.h"
#include "server/client_registry.h"
#include "server/disconnect_handler.h"
#include "server/login_handler.h"
#include "server/message_router.h"
#include "server/queue_handler.h"
#include "ws_protocol.h"

namespace network {

struct WsServer::Impl {
    session::SessionManager& session;
    events::EventBus& bus;
    db::UserStore& user_store;
    CommandQueue& command_queue;
    matchmaking::MatchmakingQueue matchmaking_queue{MatchmakingConfig::kEloMatchRange,
                                                    MatchmakingConfig::kQueueTimeoutMs};
    session::DisconnectManager disconnect_manager{DisconnectConfig::kGraceMs};
    const int port;

    ClientRegistry clients;
    LoginHandler login_handler;
    QueueHandler queue_handler;
    DisconnectHandler disconnect_handler;
    MessageRouter message_router;

    std::unique_ptr<ix::WebSocketServer> server;
    events::EventBus::SubscriptionId bus_subscription = 0;
    bool net_initialized = false;
    bool running = false;

    Impl(session::SessionManager& session_ref, events::EventBus& bus_ref, db::UserStore& store_ref,
         CommandQueue& command_queue_ref, int server_port)
        : session(session_ref),
          bus(bus_ref),
          user_store(store_ref),
          command_queue(command_queue_ref),
          port(server_port),
          login_handler(session_ref, disconnect_manager, clients),
          queue_handler(session_ref, store_ref, matchmaking_queue, clients, bus_ref, server_port),
          disconnect_handler(session_ref, disconnect_manager, queue_handler, clients, bus_ref),
          message_router(session_ref, command_queue_ref, login_handler, queue_handler,
                         disconnect_handler, clients) {}

    void handleBusEvent(const events::GameEvent& event) {
        clients.broadcast(serializeServerEvent(event));
    }

    void tickQueuesAndGrace() {
        const auto now = std::chrono::steady_clock::now();
        queue_handler.tickQueueTimeouts(now);
        disconnect_handler.tickGrace(now);
    }
};

WsServer::WsServer(session::SessionManager& session, events::EventBus& bus, db::UserStore& user_store,
                   CommandQueue& command_queue, int port)
    : impl_(std::make_unique<Impl>(session, bus, user_store, command_queue, port)) {}

WsServer::~WsServer() { stop(); }

bool WsServer::start() {
    if (impl_->running) {
        return true;
    }

    if (!ix::initNetSystem()) {
        return false;
    }
    impl_->net_initialized = true;

    impl_->server = std::make_unique<ix::WebSocketServer>(impl_->port, "127.0.0.1");
    impl_->server->disablePerMessageDeflate();

    Impl* impl_ptr = impl_.get();
    impl_->server->setOnClientMessageCallback(
        [impl_ptr](const std::shared_ptr<ix::ConnectionState>& connection_state,
                   ix::WebSocket& web_socket, const ix::WebSocketMessagePtr& message) {
            impl_ptr->message_router.handleClientMessage(connection_state, web_socket, message);
        });

    impl_->bus_subscription = impl_->bus.subscribeAny(
        [impl_ptr](const events::GameEvent& event) { impl_ptr->handleBusEvent(event); });

    if (!impl_->server->listenAndStart()) {
        impl_->bus.unsubscribe(impl_->bus_subscription);
        impl_->bus_subscription = 0;
        impl_->server.reset();
        if (impl_->net_initialized) {
            ix::uninitNetSystem();
            impl_->net_initialized = false;
        }
        return false;
    }

    impl_->running = true;
    return true;
}

void WsServer::stop() {
    if (!impl_->running && !impl_->server && !impl_->net_initialized) {
        return;
    }

    if (impl_->bus_subscription != 0) {
        impl_->bus.unsubscribe(impl_->bus_subscription);
        impl_->bus_subscription = 0;
    }

    if (impl_->server) {
        impl_->server->stop();
        impl_->server.reset();
    }

    impl_->clients.clear();

    if (impl_->net_initialized) {
        ix::uninitNetSystem();
        impl_->net_initialized = false;
    }

    impl_->running = false;
}

void WsServer::tick() {
    if (!impl_->running) {
        return;
    }

    impl_->tickQueuesAndGrace();
}

int WsServer::port() const { return impl_->port; }

}  // namespace network
