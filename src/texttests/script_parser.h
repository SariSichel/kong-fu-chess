#pragma once

#include <string>

namespace texttests {

enum class ScriptCommandType {
    Unknown,
    Click,
    Jump,
    Wait,
    PrintBoard
};

struct ScriptCommand {
    ScriptCommandType type = ScriptCommandType::Unknown;
    int x = 0;
    int y = 0;
    int wait_ms = 0;
};

class ScriptParser {
public:
    static ScriptCommand parseLine(const std::string& line);
};

}  // namespace texttests
