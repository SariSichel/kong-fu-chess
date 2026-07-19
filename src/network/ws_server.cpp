#include "ws_server.h"

#include <ixwebsocket/IXConnectionState.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketMessage.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "../model/piece.h"
#include "ws_protocol.h"

namespace network {

namespace {

std::optional<std::uint64_t> parseConnectionId(const ix::ConnectionState& connection_state) {
    try {
        return static_cast<std::uint64_t>(std::stoull(connection_state.getId()));
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::string authErrorMessage(session::AuthResult result) {
    switch (result) {
        case session::AuthResult::UnknownUser:
            return "Unknown username";
        case session::AuthResult::SlotFull:
            return "Session full";
        case session::AuthResult::AlreadyConnected:
            return "Username already connected";
        case session::AuthResult::Accepted:
        default:
            return "Login accepted";
    }
}

std::string colorLabel(model::Color color) {
    return color == model::Color::White ? "white" : "black";
}

}  // namespace

struct WsServer::Impl {
    session::SessionManager& session;
    events::EventBus& bus;
    CommandQueue& command_queue;
    const int port;

    std::unique_ptr<ix::WebSocketServer> server;
    events::EventBus::SubscriptionId bus_subscription = 0;
    bool net_initialized = false;
    bool running = false;

    std::mutex clients_mutex;
    std::unordered_map<std::uint64_t, ix::WebSocket*> clients;

    Impl(session::SessionManager& session_ref, events::EventBus& bus_ref,
         CommandQueue& command_queue_ref, int server_port)
        : session(session_ref),
          bus(bus_ref),
          command_queue(command_queue_ref),
          port(server_port) {}

    void broadcast(const std::string& json) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto& entry : clients) {
            entry.second->sendText(json);
        }
    }

    void handleBusEvent(const events::GameEvent& event) { broadcast(serializeServerEvent(event)); }

    void handleLogin(std::uint64_t connection_id, ix::WebSocket& web_socket,
                     const LoginCommand& login) {
        const session::AuthResult result = session.authenticate(connection_id, login.username);
        if (result != session::AuthResult::Accepted) {
            web_socket.sendText(serializeErrorMessage(authErrorMessage(result)));
            return;
        }

        const std::optional<model::Color> color = session.colorFor(connection_id);
        if (!color.has_value()) {
            web_socket.sendText(serializeErrorMessage("Login failed"));
            return;
        }

        web_socket.sendText(serializeLoginOk(colorLabel(*color)));
    }

    void handleClientMessage(const std::shared_ptr<ix::ConnectionState>& connection_state,
                             ix::WebSocket& web_socket, const ix::WebSocketMessagePtr& message) {
        const std::optional<std::uint64_t> connection_id = parseConnectionId(*connection_state);
        if (!connection_id.has_value()) {
            return;
        }

        if (message->type == ix::WebSocketMessageType::Open) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients[*connection_id] = &web_socket;
            return;
        }

        if (message->type == ix::WebSocketMessageType::Close) {
            session.disconnect(*connection_id);
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(*connection_id);
            return;
        }

        if (message->type != ix::WebSocketMessageType::Message || message->binary) {
            return;
        }

        const std::optional<ClientCommand> command = parseClientMessage(message->str);
        if (!command.has_value()) {
            web_socket.sendText(serializeErrorMessage("Invalid message"));
            return;
        }

        if (std::holds_alternative<LoginCommand>(*command)) {
            handleLogin(*connection_id, web_socket, std::get<LoginCommand>(*command));
            return;
        }

        if (!session.colorFor(*connection_id).has_value()) {
            web_socket.sendText(serializeErrorMessage("Not authenticated"));
            return;
        }

        command_queue.push({*connection_id, *command});
    }
};

WsServer::WsServer(session::SessionManager& session, events::EventBus& bus,
                   CommandQueue& command_queue, int port)
    : impl_(std::make_unique<Impl>(session, bus, command_queue, port)) {}

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
            impl_ptr->handleClientMessage(connection_state, web_socket, message);
        });

    impl_->bus_subscription = impl_->bus.subscribe(
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

    {
        std::lock_guard<std::mutex> lock(impl_->clients_mutex);
        impl_->clients.clear();
    }

    if (impl_->net_initialized) {
        ix::uninitNetSystem();
        impl_->net_initialized = false;
    }

    impl_->running = false;
}

int WsServer::port() const { return impl_->port; }

}  // namespace network
