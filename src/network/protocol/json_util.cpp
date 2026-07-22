#include "json_util.h"

#include <cctype>
#include <cstdlib>

namespace network {
namespace protocol {

namespace {

const char* skipWhitespace(const char* cursor) {
    while (*cursor != '\0' && std::isspace(static_cast<unsigned char>(*cursor)) != 0) {
        ++cursor;
    }
    return cursor;
}

std::optional<std::string> readJsonStringValue(const char* cursor) {
    cursor = skipWhitespace(cursor);
    if (*cursor != '"') {
        return std::nullopt;
    }

    ++cursor;
    std::string value;
    while (*cursor != '\0') {
        if (*cursor == '"') {
            return value;
        }

        if (*cursor == '\\') {
            ++cursor;
            if (*cursor == '\0') {
                return std::nullopt;
            }
            value += *cursor;
            ++cursor;
            continue;
        }

        value += *cursor;
        ++cursor;
    }

    return std::nullopt;
}

}  // namespace

std::string escapeJsonString(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            default:
                escaped += ch;
                break;
        }
    }

    return escaped;
}

std::string quoteJsonString(const std::string& value) {
    return "\"" + escapeJsonString(value) + "\"";
}

std::optional<std::string> extractJsonString(const std::string& json, const std::string& key) {
    const std::string needle = quoteJsonString(key);
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return std::nullopt;
    }

    const char* cursor = json.c_str() + key_pos + needle.size();
    cursor = skipWhitespace(cursor);
    if (*cursor != ':') {
        return std::nullopt;
    }

    ++cursor;
    return readJsonStringValue(cursor);
}

std::optional<int> extractJsonInt(const std::string& json, const std::string& key) {
    const std::string needle = quoteJsonString(key);
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return std::nullopt;
    }

    const char* cursor = json.c_str() + key_pos + needle.size();
    cursor = skipWhitespace(cursor);
    if (*cursor != ':') {
        return std::nullopt;
    }

    ++cursor;
    cursor = skipWhitespace(cursor);
    char* end = nullptr;
    const long value = std::strtol(cursor, &end, 10);
    if (cursor == end) {
        return std::nullopt;
    }
    return static_cast<int>(value);
}

}  // namespace protocol
}  // namespace network
