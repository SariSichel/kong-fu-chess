#pragma once

#include <optional>
#include <string>

namespace network {
namespace protocol {

std::string escapeJsonString(const std::string& value);
std::string quoteJsonString(const std::string& value);
std::optional<std::string> extractJsonString(const std::string& json, const std::string& key);
std::optional<int> extractJsonInt(const std::string& json, const std::string& key);

}  // namespace protocol
}  // namespace network
