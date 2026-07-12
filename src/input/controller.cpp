#include "controller.h"

#include "../engine/game_engine.h"
#include "../rules/rule_engine.h"
#include "board_mapper.h"

namespace input {

void Controller::clearSelection() {
    selected_square_ = model::Position(-1, -1);
}

void Controller::handleClick(engine::GameEngine& gameEngine, int x, int y) {
    if (gameEngine.isGameOver()) {
        return;
    }

    model::Board& board = gameEngine.board();
    const model::Position clicked = BoardMapper::toPosition(x, y);
    if (clicked.row < 0) {
        return;
    }
    if (!board.inBounds(clicked)) {
        return;
    }

    const model::Piece& clickedPiece = board.cell(clicked);

    if (!hasSelection()) {
        if (clickedPiece.isEmpty()) {
            return;
        }
        selected_square_ = clicked;
        return;
    }

    if (!clickedPiece.isEmpty()) {
        const model::Piece& selectedPiece = board.cell(selected_square_);
        if (clickedPiece.color() == selectedPiece.color()) {
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

    if (!rules::RuleEngine::validateMove(board, selected_square_, clicked)) {
        return;
    }

    if (!gameEngine.requestMove(selected_square_, clicked)) {
        return;
    }

    clearSelection();
}

void Controller::handleJump(engine::GameEngine& gameEngine, int x, int y) {
    const model::Position pos = BoardMapper::toPosition(x, y);
    if (pos.row < 0) {
        return;
    }

    if (gameEngine.requestJump(pos)) {
        clearSelection();
    }
}

}  // namespace input
