#include "ws_server.h"

#include <ixwebsocket/IXConnectionState.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketMessage.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "../constants.h"
#include "../db/user_store.h"
#include "../session/disconnect_manager.h"
#include "ws_protocol.h"

namespace network {

namespace {

constexpr const char* kQueueTimeoutMessage = "No opponent found within 60 seconds.";

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

}  // namespace

struct WsServer::Impl {
    session::SessionManager& session;
    events::EventBus& bus;
    db::UserStore& user_store;
    CommandQueue& command_queue;
    matchmaking::MatchmakingQueue matchmaking_queue{MatchmakingConfig::kEloMatchRange,
                                                    MatchmakingConfig::kQueueTimeoutMs};
    session::DisconnectManager disconnect_manager{DisconnectConfig::kGraceMs};
    const int port;

    std::unique_ptr<ix::WebSocketServer> server;
    events::EventBus::SubscriptionId bus_subscription = 0;
    bool net_initialized = false;
    bool running = false;

    std::mutex clients_mutex;
    std::unordered_map<std::uint64_t, ix::WebSocket*> clients;

    Impl(session::SessionManager& session_ref, events::EventBus& bus_ref, db::UserStore& store_ref,
         CommandQueue& command_queue_ref, int server_port)
        : session(session_ref),
          bus(bus_ref),
          user_store(store_ref),
          command_queue(command_queue_ref),
          port(server_port) {}

    void sendToConnection(std::uint64_t connection_id, const std::string& json) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        const auto it = clients.find(connection_id);
        if (it != clients.end() && it->second != nullptr) {
            it->second->sendText(json);
        }
    }

    void broadcast(const std::string& json) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto& entry : clients) {
            entry.second->sendText(json);
        }
    }

    void handleBusEvent(const events::GameEvent& event) { broadcast(serializeServerEvent(event)); }

    void handleLogin(std::uint64_t connection_id, ix::WebSocket& web_socket,
                     const LoginCommand& login) {
        if (disconnect_manager.isInGrace(login.username)) {
            const session::AuthResult reconnect_result =
                session.reconnect(connection_id, login.username);
            if (reconnect_result != session::AuthResult::Accepted) {
                web_socket.sendText(serializeErrorMessage(authErrorMessage(reconnect_result)));
                return;
            }

            disconnect_manager.onReconnect(login.username);
            broadcast(serializePlayerReconnected(login.username));

            const std::optional<model::Color> color = session.colorFor(connection_id);
            if (!color.has_value()) {
                web_socket.sendText(serializeErrorMessage("Reconnect failed"));
                return;
            }

            web_socket.sendText(serializeLoginOk(colorLabel(*color)));
            web_socket.sendText(serializeServerEvent(session.rosterSnapshot()));
            return;
        }

        const session::AuthResult result = session.authenticate(connection_id, login.username);
        if (result != session::AuthResult::Accepted) {
            web_socket.sendText(serializeErrorMessage(authErrorMessage(result)));
            return;
        }

        const std::optional<model::Color> color = session.colorFor(connection_id);
        if (!color.has_value()) {
            web_socket.sendText(serializeLoginOk("none"));
            return;
        }

        web_socket.sendText(serializeLoginOk(colorLabel(*color)));
    }

    void notifyMatch(const matchmaking::MatchResult& match) {
        session.beginMatch(match);

        sendToConnection(match.white.connection_id,
                         serializeMatchFound("white", match.black.username, port));
        sendToConnection(match.black.connection_id,
                         serializeMatchFound("black", match.white.username, port));

        bus.publish(session.rosterSnapshot());
    }

    void handleJoinQueue(std::uint64_t connection_id, ix::WebSocket& web_socket) {
        if (!session.usernameFor(connection_id).has_value()) {
            web_socket.sendText(serializeErrorMessage("Not authenticated"));
            return;
        }

        if (session.phase() == session::SessionPhase::InGame) {
            if (session.colorFor(connection_id).has_value() || session.isReady()) {
                web_socket.sendText(serializeErrorMessage("Already in game"));
                return;
            }
        }

        const std::string username = *session.usernameFor(connection_id);
        int elo = 1200;
        if (const std::optional<db::UserRecord> record = user_store.find(username);
            record.has_value()) {
            elo = record->elo;
        }

        matchmaking::QueueEntry entry{connection_id, username, elo,
                                      std::chrono::steady_clock::now()};

        const std::optional<matchmaking::MatchResult> match = matchmaking_queue.enqueue(entry);
        web_socket.sendText(serializeQueueJoined());

        if (match.has_value()) {
            notifyMatch(*match);
        }
    }

    void handleLeaveQueue(std::uint64_t connection_id, ix::WebSocket& web_socket) {
        if (!session.usernameFor(connection_id).has_value()) {
            web_socket.sendText(serializeErrorMessage("Not authenticated"));
            return;
        }

        matchmaking_queue.remove(connection_id);
        web_socket.sendText(serializeQueueLeft());
    }

    void removeFromQueue(std::uint64_t connection_id) { matchmaking_queue.remove(connection_id); }

    void handleDisconnect(std::uint64_t connection_id) {
        if (session.phase() == session::SessionPhase::InGame) {
            const std::optional<std::string> username = session.usernameFor(connection_id);
            const std::optional<model::Color> color = session.colorFor(connection_id);
            if (username.has_value() && color.has_value()) {
                const auto now = std::chrono::steady_clock::now();
                session::DisconnectGraceEntry entry;
                entry.username = *username;
                entry.color = *color;
                entry.connection_id = connection_id;
                entry.disconnected_at = now;
                entry.grace_until = now + std::chrono::milliseconds(DisconnectConfig::kGraceMs);

                disconnect_manager.onDisconnect(entry);
                session.reserveDisconnectedSlot(connection_id, *username, *color);
                broadcast(serializePlayerDisconnected(*username, colorLabel(*color),
                                                      DisconnectConfig::kGraceSeconds));

                std::lock_guard<std::mutex> lock(clients_mutex);
                clients.erase(connection_id);
                return;
            }
        }

        removeFromQueue(connection_id);
        session.disconnect(connection_id);

        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(connection_id);
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
            handleDisconnect(*connection_id);
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

        if (std::holds_alternative<JoinQueueCommand>(*command)) {
            handleJoinQueue(*connection_id, web_socket);
            return;
        }

        if (std::holds_alternative<LeaveQueueCommand>(*command)) {
            handleLeaveQueue(*connection_id, web_socket);
            return;
        }

        if (!session.colorFor(*connection_id).has_value()) {
            web_socket.sendText(serializeErrorMessage("Not authenticated"));
            return;
        }

        command_queue.push({*connection_id, *command});
    }

    void tickQueuesAndGrace() {
        const auto now = std::chrono::steady_clock::now();

        const std::vector<matchmaking::QueueEntry> expired = matchmaking_queue.tick(now);
        for (const matchmaking::QueueEntry& entry : expired) {
            sendToConnection(entry.connection_id, serializeQueueTimeout(kQueueTimeoutMessage));
        }

        const std::optional<model::Color> winner = disconnect_manager.tick(now);
        if (winner.has_value()) {
            bus.publish(events::GameEnded{*winner});
            session.resetToLobby();
        }
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

void WsServer::tick() {
    if (!impl_->running) {
        return;
    }

    impl_->tickQueuesAndGrace();
}

int WsServer::port() const { return impl_->port; }

}  // namespace network
