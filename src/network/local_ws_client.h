#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "../model/piece.h"
#include "../model/position.h"

namespace network {

class LocalWsClient {
public:
    using MessageHandler = std::function<void(const std::string& json)>;

    LocalWsClient(std::string url, std::string username);
    ~LocalWsClient();

    LocalWsClient(const LocalWsClient&) = delete;
    LocalWsClient& operator=(const LocalWsClient&) = delete;

    bool connectAndLogin(int timeout_ms = 5000);
    void disconnect();

    bool isLoggedIn() const { return logged_in_.load(); }
    std::optional<model::Color> assignedColor() const;

    void setMessageHandler(MessageHandler handler);

    bool sendMove(const model::Position& from, const model::Position& to);
    bool sendJump(const model::Position& square);
    bool sendJoinQueue();
    bool sendLeaveQueue();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::string url_;
    std::string username_;
    std::atomic<bool> logged_in_{false};
    mutable std::mutex state_mutex_;
    std::optional<model::Color> assigned_color_;
    std::mutex handler_mutex_;
    MessageHandler message_handler_;
};

}  // namespace network
