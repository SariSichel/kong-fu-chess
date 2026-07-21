#pragma once

#include <string>

#include "../model/piece.h"

namespace session {

struct MatchInfo {
    model::Color color = model::Color::White;
    std::string opponent;
    int port = 0;
    std::string white_user;
    std::string black_user;
};

}  // namespace session
