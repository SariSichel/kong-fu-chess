#include "script_parser.h"

#include <sstream>

#include "../config/game_config.h"
#include "../io/board_parser.h"

namespace texttests {

ScriptCommand ScriptParser::parseLine(const std::string& line) {
    ScriptCommand command;
    const std::string trimmed = io::BoardParser::trimLine(line);
    if (trimmed.empty()) {
        return command;
    }

    std::stringstream ss(trimmed);
    std::string cmd;
    ss >> cmd;

    if (cmd == Commands::kClick) {
        if (ss >> command.x >> command.y) {
            command.type = ScriptCommandType::Click;
        }
    } else if (cmd == Commands::kJump) {
        if (ss >> command.x >> command.y) {
            command.type = ScriptCommandType::Jump;
        }
    } else if (cmd == Commands::kWait) {
        if (ss >> command.wait_ms) {
            command.type = ScriptCommandType::Wait;
        }
    } else if (cmd == Commands::kPrint) {
        std::string subcmd;
        if (ss >> subcmd && subcmd == Commands::kPrintBoard) {
            command.type = ScriptCommandType::PrintBoard;
        }
    }

    return command;
}

}  // namespace texttests
