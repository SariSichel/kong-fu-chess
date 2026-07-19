#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include "../model/position.h"

namespace network {

class LocalWsClient {
public:
    LocalWsClient(std::string url, std::string username);
    ~LocalWsClient();

    LocalWsClient(const LocalWsClient&) = delete;
    LocalWsClient& operator=(const LocalWsClient&) = delete;

    bool connectAndLogin(int timeout_ms = 5000);
    void disconnect();

    bool isLoggedIn() const { return logged_in_.load(); }

    bool sendMove(const model::Position& from, const model::Position& to);
    bool sendJump(const model::Position& square);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::string url_;
    std::string username_;
    std::atomic<bool> logged_in_{false};
};

}  // namespace network
