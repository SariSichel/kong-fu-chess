#include "host_election.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace app {

#ifdef _WIN32
// On Windows, IXWebSocket binds its listen socket with SO_REUSEADDR, which lets
// multiple processes bind the same 127.0.0.1:port. Relying on bind() failure for
// host election is therefore unreliable and can produce two "hosts" (a split-brain
// where one process owns an empty authoritative engine and freezes). We elect a
// single host out-of-band via a named mutex: the first process to create it hosts,
// everyone else runs as a client. The mutex object lives as long as any handle is
// open, so if the host exits/crashes a later instance can win election.
static HANDLE g_host_mutex = nullptr;

bool acquireHostElection() {
    g_host_mutex = CreateMutexW(nullptr, FALSE, L"Local\\KongFuChessHost");
    if (g_host_mutex == nullptr) {
        return false;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(g_host_mutex);
        g_host_mutex = nullptr;
        return false;
    }

    return true;
}

void releaseHostElection() {
    if (g_host_mutex != nullptr) {
        CloseHandle(g_host_mutex);
        g_host_mutex = nullptr;
    }
}
#else
bool acquireHostElection() { return true; }
void releaseHostElection() {}
#endif

}  // namespace app
