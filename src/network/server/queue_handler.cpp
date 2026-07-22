#include "queue_handler.h"

#include "../ws_protocol.h"

#include <chrono>

namespace network {

namespace {

constexpr const char* kQueueTimeoutMessage = "No opponent found within 60 seconds.";

}  // namespace

QueueHandler::QueueHandler(session::SessionManager& session, db::UserStore& user_store,
                           matchmaking::MatchmakingQueue& matchmaking_queue,
                           ClientRegistry& clients, events::EventBus& bus, int port)
    : session_(session),
      user_store_(user_store),
      matchmaking_queue_(matchmaking_queue),
      clients_(clients),
      bus_(bus),
      port_(port) {}

void QueueHandler::notifyMatch(const matchmaking::MatchResult& match) {
    session_.beginMatch(match);

    clients_.sendToConnection(match.white.connection_id,
                              serializeMatchFound("white", match.black.username, port_));
    clients_.sendToConnection(match.black.connection_id,
                              serializeMatchFound("black", match.white.username, port_));

    bus_.publish(session_.rosterSnapshot());
}

void QueueHandler::handleJoinQueue(std::uint64_t connection_id, ix::WebSocket& web_socket) {
    if (!session_.usernameFor(connection_id).has_value()) {
        web_socket.sendText(serializeErrorMessage("Not authenticated"));
        return;
    }

    if (session_.phase() == session::SessionPhase::InGame) {
        if (session_.colorFor(connection_id).has_value() || session_.isReady()) {
            web_socket.sendText(serializeErrorMessage("Already in game"));
            return;
        }
    }

    const std::string username = *session_.usernameFor(connection_id);
    int elo = 1200;
    if (const std::optional<db::UserRecord> record = user_store_.find(username);
        record.has_value()) {
        elo = record->elo;
    }

    matchmaking::QueueEntry entry{connection_id, username, elo, std::chrono::steady_clock::now()};

    const std::optional<matchmaking::MatchResult> match = matchmaking_queue_.enqueue(entry);
    web_socket.sendText(serializeQueueJoined());

    if (match.has_value()) {
        notifyMatch(*match);
    }
}

void QueueHandler::handleLeaveQueue(std::uint64_t connection_id, ix::WebSocket& web_socket) {
    if (!session_.usernameFor(connection_id).has_value()) {
        web_socket.sendText(serializeErrorMessage("Not authenticated"));
        return;
    }

    matchmaking_queue_.remove(connection_id);
    web_socket.sendText(serializeQueueLeft());
}

void QueueHandler::removeFromQueue(std::uint64_t connection_id) {
    matchmaking_queue_.remove(connection_id);
}

void QueueHandler::tickQueueTimeouts(std::chrono::steady_clock::time_point now) {
    const std::vector<matchmaking::QueueEntry> expired = matchmaking_queue_.tick(now);
    for (const matchmaking::QueueEntry& entry : expired) {
        clients_.sendToConnection(entry.connection_id, serializeQueueTimeout(kQueueTimeoutMessage));
    }
}

}  // namespace network
