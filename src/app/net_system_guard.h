#pragma once

#include <ixwebsocket/IXNetSystem.h>

namespace app {

// Winsock (WSAStartup) must be initialized before any socket is used on Windows.
// IXWebSocket does not do this automatically, so a client-only process (one that
// never starts a WsServer) would otherwise fail every socket call. This guard
// initializes networking for the whole process lifetime, independent of whether we
// host. WSAStartup/WSACleanup are reference counted, so this coexists with the
// server's own init/uninit.
class NetSystemGuard {
public:
    NetSystemGuard() : initialized_(ix::initNetSystem()) {}
    ~NetSystemGuard() {
        if (initialized_) {
            ix::uninitNetSystem();
        }
    }

    NetSystemGuard(const NetSystemGuard&) = delete;
    NetSystemGuard& operator=(const NetSystemGuard&) = delete;

    bool ok() const { return initialized_; }

private:
    bool initialized_ = false;
};

}  // namespace app
