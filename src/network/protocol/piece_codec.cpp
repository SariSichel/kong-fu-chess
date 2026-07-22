#include "piece_codec.h"

namespace network {
namespace protocol {

bool parsePieceLabel(const std::string& label, model::PieceType& type, model::Color& color) {
    if (label.size() != 2) {
        return false;
    }

    if (label[0] == 'w') {
        color = model::Color::White;
    } else if (label[0] == 'b') {
        color = model::Color::Black;
    } else {
        return false;
    }

    switch (label[1]) {
        case 'K':
            type = model::PieceType::King;
            return true;
        case 'Q':
            type = model::PieceType::Queen;
            return true;
        case 'R':
            type = model::PieceType::Rook;
            return true;
        case 'B':
            type = model::PieceType::Bishop;
            return true;
        case 'N':
            type = model::PieceType::Knight;
            return true;
        case 'P':
            type = model::PieceType::Pawn;
            return true;
        default:
            return false;
    }
}

}  // namespace protocol
}  // namespace network
