#include "../ws_protocol.h"

namespace network {

model::Color colorFromLabel(const std::string& label) {
    return label == "black" ? model::Color::Black : model::Color::White;
}

std::string colorLabel(model::Color color) {
    return color == model::Color::White ? "white" : "black";
}

}  // namespace network
