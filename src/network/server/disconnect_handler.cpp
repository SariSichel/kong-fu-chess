#include "disconnect_handler.h"

#include "../../config/network_config.h"
#include "../ws_protocol.h"
#include "queue_handler.h"

namespace network {

DisconnectHandler::DisconnectHandler(session::SessionManager& session,
                                     session::DisconnectManager& disconnect_manager,
                                     QueueHandler& queue_handler, ClientRegistry& clients,
                                     events::EventBus& bus)
    : session_(session),
      disconnect_manager_(disconnect_manager),
      queue_handler_(queue_handler),
      clients_(clients),
      bus_(bus) {}

void DisconnectHandler::handleDisconnect(std::uint64_t connection_id) {
    if (session_.phase() == session::SessionPhase::InGame) {
        const std::optional<std::string> username = session_.usernameFor(connection_id);
        const std::optional<model::Color> color = session_.colorFor(connection_id);
        if (username.has_value() && color.has_value()) {
            const auto now = std::chrono::steady_clock::now();
            session::DisconnectGraceEntry entry;
            entry.username = *username;
            entry.color = *color;
            entry.connection_id = connection_id;
            entry.disconnected_at = now;
            entry.grace_until = now + std::chrono::milliseconds(DisconnectConfig::kGraceMs);

            disconnect_manager_.onDisconnect(entry);
            session_.reserveDisconnectedSlot(connection_id, *username, *color);
            clients_.broadcast(serializePlayerDisconnected(*username, colorLabel(*color),
                                                           DisconnectConfig::kGraceSeconds));
            clients_.removeClient(connection_id);
            return;
        }
    }

    queue_handler_.removeFromQueue(connection_id);
    session_.disconnect(connection_id);
    clients_.removeClient(connection_id);
}

void DisconnectHandler::tickGrace(std::chrono::steady_clock::time_point now) {
    const std::optional<model::Color> winner = disconnect_manager_.tick(now);
    if (winner.has_value()) {
        bus_.publish(events::GameEnded{*winner});
        session_.resetToLobby();
    }
}

}  // namespace network
