#include "local_ws_client.h"

#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketMessage.h>

#include <chrono>
#include <memory>
#include <thread>

#include "../io/algebraic.h"
#include "ws_protocol.h"

namespace network {

struct LocalWsClient::Impl {
    ix::WebSocket web_socket;
};

LocalWsClient::LocalWsClient(std::string url, std::string username)
    : impl_(std::make_unique<Impl>()),
      url_(std::move(url)),
      username_(std::move(username)) {}

LocalWsClient::~LocalWsClient() { disconnect(); }

void LocalWsClient::setMessageHandler(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    message_handler_ = std::move(handler);
}

bool LocalWsClient::connectAndLogin(int timeout_ms) {
    if (logged_in_.load()) {
        return true;
    }

    logged_in_.store(false);

    impl_->web_socket.setUrl(url_);
    impl_->web_socket.disableAutomaticReconnection();

    impl_->web_socket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& message) {
        if (message->type == ix::WebSocketMessageType::Open) {
            impl_->web_socket.sendText(serializeLoginCommand(username_));
            return;
        }

        if (message->type != ix::WebSocketMessageType::Message || message->binary) {
            return;
        }

        if (parseServerMessageType(message->str) == ServerMessageType::LoginOk) {
            logged_in_.store(true);
        }

        MessageHandler handler;
        {
            std::lock_guard<std::mutex> lock(handler_mutex_);
            handler = message_handler_;
        }
        if (handler) {
            handler(message->str);
        }
    });

    impl_->web_socket.start();

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        if (logged_in_.load()) {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    disconnect();
    return false;
}

void LocalWsClient::disconnect() {
    logged_in_.store(false);
    impl_->web_socket.stop();
}

bool LocalWsClient::sendMove(const model::Position& from, const model::Position& to) {
    if (!logged_in_.load() || from.row < 0 || to.row < 0) {
        return false;
    }

    impl_->web_socket.sendText(
        serializeMoveCommand(io::positionToAlgebraic(from), io::positionToAlgebraic(to)));
    return true;
}

bool LocalWsClient::sendJump(const model::Position& square) {
    if (!logged_in_.load() || square.row < 0) {
        return false;
    }

    impl_->web_socket.sendText(serializeJumpCommand(io::positionToAlgebraic(square)));
    return true;
}

bool LocalWsClient::sendJoinQueue() {
    if (!logged_in_.load()) {
        return false;
    }

    impl_->web_socket.sendText(serializeJoinQueueCommand());
    return true;
}

bool LocalWsClient::sendLeaveQueue() {
    if (!logged_in_.load()) {
        return false;
    }

    impl_->web_socket.sendText(serializeLeaveQueueCommand());
    return true;
}

}  // namespace network
