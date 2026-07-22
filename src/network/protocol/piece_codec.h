#pragma once

#include "../../model/piece.h"

#include <string>

namespace network {
namespace protocol {

bool parsePieceLabel(const std::string& label, model::PieceType& type, model::Color& color);

}  // namespace protocol
}  // namespace network
