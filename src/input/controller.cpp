#include "controller.h"

#include "../engine/game_engine.h"
#include "../rules/rule_engine.h"
#include "board_mapper.h"

namespace input {

namespace {

bool canControlPiece(const model::Piece& piece, std::optional<model::Color> player_color) {
    if (!player_color.has_value()) {
        return true;
    }

    return !piece.isEmpty() && piece.color() == *player_color;
}

}  // namespace

void Controller::clearSelection() {
    selected_square_ = model::Position(-1, -1);
}

void Controller::setDispatch(MoveDispatch move_dispatch, JumpDispatch jump_dispatch) {
    move_dispatch_ = std::move(move_dispatch);
    jump_dispatch_ = std::move(jump_dispatch);
}

bool Controller::requestMove(engine::GameEngine& gameEngine, const model::Position& from,
                             const model::Position& to) {
    if (move_dispatch_) {
        return move_dispatch_(from, to);
    }

    return gameEngine.requestMove(from, to);
}

bool Controller::requestJump(engine::GameEngine& gameEngine, const model::Position& square) {
    if (jump_dispatch_) {
        return jump_dispatch_(square);
    }

    return gameEngine.requestJump(square);
}

void Controller::handleClick(engine::GameEngine& gameEngine, int x, int y,
                             std::optional<model::Color> player_color) {
    if (gameEngine.isGameOver()) {
        return;
    }

    model::Board& board = gameEngine.board();
    const model::Position clicked = BoardMapper::toPosition(x, y);
    if (clicked.row < 0 || !board.inBounds(clicked)) {
        if (hasSelection()) {
            clearSelection();
        }
        return;
    }

    const model::Piece& clickedPiece = board.cell(clicked);

    if (!hasSelection()) {
        if (clickedPiece.isEmpty()) {
            return;
        }
        if (!canControlPiece(clickedPiece, player_color)) {
            return;
        }
        selected_square_ = clicked;
        return;
    }

    const model::Piece& selectedPiece = board.cell(selected_square_);
    if (!canControlPiece(selectedPiece, player_color)) {
        clearSelection();
        return;
    }

    if (!clickedPiece.isEmpty()) {
        if (clickedPiece.color() == selectedPiece.color()) {
            if (!canControlPiece(clickedPiece, player_color)) {
                return;
            }
            selected_square_ = clicked;
            return;
        }
    }

    if (gameEngine.isBusyAt(selected_square_)) {
        if (gameEngine.isActiveMoveStartTick(selected_square_)) {
            model::Position destination;
            if (gameEngine.getActiveMoveDestination(selected_square_, destination)) {
                gameEngine.queuePremove(selected_square_, destination, clicked);
            }
        }
        clearSelection();
        return;
    }

    if (clickedPiece.isEmpty()) {
        if (rules::RuleEngine::validateMove(board, selected_square_, clicked)) {
            requestMove(gameEngine, selected_square_, clicked);
        }
        clearSelection();
        return;
    }

    if (!rules::RuleEngine::validateMove(board, selected_square_, clicked)) {
        return;
    }

    if (!requestMove(gameEngine, selected_square_, clicked)) {
        return;
    }

    clearSelection();
}

void Controller::handleJump(engine::GameEngine& gameEngine, int x, int y,
                            std::optional<model::Color> player_color) {
    const model::Position pos = BoardMapper::toPosition(x, y);
    if (pos.row < 0) {
        return;
    }

    if (!canControlPiece(gameEngine.board().cell(pos), player_color)) {
        return;
    }

    if (requestJump(gameEngine, pos)) {
        clearSelection();
    }
}

}  // namespace input
