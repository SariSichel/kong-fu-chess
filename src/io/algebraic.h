#pragma once

#include <string>

#include "../model/position.h"

namespace io {

std::string positionToAlgebraic(const model::Position& pos);
model::Position algebraicToPosition(const std::string& square);

}  // namespace io
