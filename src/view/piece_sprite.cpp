#include "piece_sprite.h"

#include <string>

#include "../constants.h"

namespace view {

namespace {

constexpr const char* kPiecesRoot =
    R"(C:\Users\saris\OneDrive\Documents\bootcamp\CTD26\pieces2)";

char pieceTypeToChar(model::PieceType type) {
    switch (type) {
        case model::PieceType::King:
            return BoardTokens::kKing;
        case model::PieceType::Queen:
            return BoardTokens::kQueen;
        case model::PieceType::Rook:
            return BoardTokens::kRook;
        case model::PieceType::Bishop:
            return BoardTokens::kBishop;
        case model::PieceType::Knight:
            return BoardTokens::kKnight;
        case model::PieceType::Pawn:
            return BoardTokens::kPawn;
        case model::PieceType::Empty:
        default:
            return BoardTokens::kEmpty;
    }
}

}  // namespace

std::string pieceToSpriteCode(const model::Piece& piece) {
    const char typeChar = pieceTypeToChar(piece.type());
    const char colorChar = piece.color() == model::Color::White ? 'W' : 'B';
    return std::string({typeChar, colorChar});
}

std::string idleSpritePath(const std::string& pieceCode) {
    return std::string(kPiecesRoot) + "/" + pieceCode + "/states/idle/sprites/1.png";
}

}  // namespace view
