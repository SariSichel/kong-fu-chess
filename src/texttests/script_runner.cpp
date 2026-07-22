#include "script_runner.h"

#include <istream>
#include <ostream>
#include <string>

#include "../config/game_config.h"
#include "../engine/game_engine.h"
#include "../input/controller.h"
#include "../io/board_parser.h"
#include "../io/board_printer.h"
#include "script_parser.h"

namespace texttests {

struct ScriptRunner::Impl {
    engine::GameEngine gameEngine;
    input::Controller controller;
};

ScriptRunner::ScriptRunner() : impl_(std::make_unique<Impl>()) {}

ScriptRunner::~ScriptRunner() = default;

void ScriptRunner::processLine(const std::string& line, std::ostream& out) {
    const ScriptCommand command = ScriptParser::parseLine(line);

    switch (command.type) {
        case ScriptCommandType::Click:
            impl_->controller.handleClick(impl_->gameEngine, command.x, command.y);
            break;
        case ScriptCommandType::Jump:
            impl_->controller.handleJump(impl_->gameEngine, command.x, command.y);
            break;
        case ScriptCommandType::Wait:
            impl_->gameEngine.advanceTime(command.wait_ms);
            break;
        case ScriptCommandType::PrintBoard:
            impl_->gameEngine.advanceTime(0);
            io::BoardPrinter::print(impl_->gameEngine.board(), out);
            out.flush();
            break;
        case ScriptCommandType::Unknown:
        default:
            break;
    }
}

void ScriptRunner::run(std::istream& in, std::ostream& out) {
    const io::ParseResult result = io::BoardParser::parseFromInput(in, impl_->gameEngine.board());
    if (result != io::ParseResult::OK) {
        switch (result) {
            case io::ParseResult::ERROR_UNKNOWN_TOKEN:
                out << ErrorMessages::kUnknownToken;
                break;
            case io::ParseResult::ERROR_ROW_WIDTH_MISMATCH:
                out << ErrorMessages::kRowWidthMismatch;
                break;
            default:
                break;
        }
        out.flush();
        return;
    }

    impl_->gameEngine.reset();

    std::string line;
    while (std::getline(in, line)) {
        processLine(line, out);
    }
}

}  // namespace texttests
