#pragma once

#include <string>

#include "../model/piece.h"

namespace view {

std::string pieceToSpriteCode(const model::Piece& piece);
std::string idleSpritePath(const std::string& pieceCode);

}  // namespace view
