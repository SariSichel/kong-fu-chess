#include "network_input.h"

#include "../engine/game_engine.h"
#include "../io/algebraic.h"

namespace network {

NetworkInputHandler::NetworkInputHandler(session::SessionManager& session,
                                         engine::GameEngine& game_engine)
    : session_(session), game_engine_(game_engine) {}

session::AuthResult NetworkInputHandler::handleLogin(std::uint64_t connection_id,
                                                     const std::string& username) {
    return session_.authenticate(connection_id, username);
}

bool NetworkInputHandler::canControlSquare(std::uint64_t connection_id,
                                           const model::Position& square) const {
    const std::optional<model::Color> player_color = session_.colorFor(connection_id);
    if (!player_color.has_value()) {
        return false;
    }

    const model::Board& board = game_engine_.board();
    if (!board.inBounds(square)) {
        return false;
    }

    const model::Piece& piece = board.cell(square);
    if (piece.isEmpty()) {
        return false;
    }

    return piece.color() == *player_color;
}

bool NetworkInputHandler::handleMove(std::uint64_t connection_id, const model::Position& from,
                                     const model::Position& to) {
    if (!canControlSquare(connection_id, from)) {
        return false;
    }

    return game_engine_.requestMove(from, to);
}

bool NetworkInputHandler::handleJump(std::uint64_t connection_id,
                                     const model::Position& square) {
    if (!canControlSquare(connection_id, square)) {
        return false;
    }

    return game_engine_.requestJump(square);
}

bool NetworkInputHandler::handleMove(std::uint64_t connection_id, const std::string& from,
                                     const std::string& to) {
    const model::Position from_pos = io::algebraicToPosition(from);
    const model::Position to_pos = io::algebraicToPosition(to);
    if (from_pos.row < 0 || to_pos.row < 0) {
        return false;
    }

    return handleMove(connection_id, from_pos, to_pos);
}

bool NetworkInputHandler::handleJump(std::uint64_t connection_id, const std::string& square) {
    const model::Position square_pos = io::algebraicToPosition(square);
    if (square_pos.row < 0) {
        return false;
    }

    return handleJump(connection_id, square_pos);
}

}  // namespace network
