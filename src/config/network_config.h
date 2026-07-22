#ifndef CONFIG_NETWORK_CONFIG_H
#define CONFIG_NETWORK_CONFIG_H

namespace NetworkConfig {
constexpr int kDefaultPort = 8765;
constexpr int kMaxPlayers = 2;
}  // namespace NetworkConfig

namespace MatchmakingConfig {
constexpr int kEloMatchRange = 100;
constexpr int kQueueTimeoutMs = 60000;
}  // namespace MatchmakingConfig

namespace DisconnectConfig {
constexpr int kGraceMs = 20000;
constexpr int kGraceSeconds = 20;
}  // namespace DisconnectConfig

#endif
