#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <sstream>
#include <string>

#include "board.h"
#include "board_serializer.h"
#include "constants.h"
#include "game_state.h"

namespace {

ParseResult parseInput(const std::string& input, Board& board) {
    std::istringstream in(input);
    return BoardSerializer::parseFromInput(in, board);
}

std::string runInput(const std::string& input) {
    std::istringstream in(input);
    std::ostringstream out;
    Board::run(in, out);
    return out.str();
}

Board makeCommonRouteBoard() {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    return board;
}

void clickSquare(GameState& state, Board& board, int row, int col) {
    const int x = col * GameConfig::kClickCellSize + GameConfig::kClickCellSize / 2;
    const int y = row * GameConfig::kClickCellSize + GameConfig::kClickCellSize / 2;
    state.handleClick(board, x, y);
}

bool hasRookAt(const Board& board, int row, int col) {
    const Piece& piece = board.cell(row, col);
    return piece.type() == PieceType::Rook && piece.color() == Color::White;
}

PieceMovementState movementStateAt(const Board& board, int row, int col) {
    return board.cell(row, col).movementState();
}

bool isPieceMovingAt(const Board& board, int row, int col) {
    return movementStateAt(board, row, col) == PieceMovementState::Moving;
}

int pieceDestinationRow(const Board& board, int row, int col) {
    return board.cell(row, col).destinationRow();
}

int pieceDestinationCol(const Board& board, int row, int col) {
    return board.cell(row, col).destinationCol();
}

constexpr int kSquareA_R = 0;
constexpr int kSquareA_C = 0;
constexpr int kSquareB_R = 0;
constexpr int kSquareB_C = 2;
constexpr int kSquareC_R = 0;
constexpr int kSquareC_C = 3;

constexpr long kMoveAToBDurationMs = 2 * GameConfig::kMoveDurationMs;
constexpr long kMoveBToCDurationMs = 1 * GameConfig::kMoveDurationMs;

}  // namespace

TEST_CASE("board parsing and printing") {
    Board board;

    CHECK(parseInput(" Board:\nwP . wK\nwP . wP\nCommands:", board) == ParseResult::OK);
    CHECK(board.rows() == 2);
    CHECK(board.cols() == 3);

    std::ostringstream out;
    BoardSerializer::print(board, out);
    CHECK(out.str() == "wP . wK\nwP . wP\n");
}

TEST_CASE("parse errors") {
    Board board;

    CHECK(parseInput("Board:\ninvalid\nCommands:", board) == ParseResult::ERROR_UNKNOWN_TOKEN);
    CHECK(parseInput("Board:\nwP .\nwP . wP\nCommands:", board) ==
          ParseResult::ERROR_ROW_WIDTH_MISMATCH);
}

TEST_CASE("basic click and command integration") {
    CHECK(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 150 150\nwait "
                   "1000\nprint board") == ". . .\n. wK .\n. . .\n");
    CHECK(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 150 150\nclick 250 250\nwait "
                   "1000\nprint board") == "wK . .\n. . .\n. . .\n");
    CHECK(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 350 50\nclick -10 50\nprint "
                   "board") == "wK . .\n. . .\n. . .\n");
    CHECK(runInput(" Board:\nwR . wK\n. . .\nCommands:\nclick 50 50\nclick 250 50\nclick 250 "
                   "150\nwait 1000\nprint board") == "wR . .\n. . wK\n");
    CHECK(runInput(" Board:\nwK xZ\n. .\nCommands:\nprint board") == "ERROR UNKNOWN_TOKEN");
    CHECK(runInput(" Board:\nwK . .\n. bK\nCommands:\nprint board") ==
          "ERROR ROW_WIDTH_MISMATCH");
}

TEST_CASE("King movement") {
    CHECK(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 150 50\nwait "
                   "1000\nprint board") == ". wK .\n. . .\n. . .\n");
    CHECK(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 250 50\nwait "
                   "1000\nprint board") == "wK . .\n. . .\n. . .\n");
}

TEST_CASE("Rook movement") {
    CHECK(runInput(" Board:\nwR . . .\n. . . .\nCommands:\nclick 50 50\nclick 350 50\nwait "
                   "3000\nprint board") == ". . . wR\n. . . .\n");
    CHECK(runInput(" Board:\nwR . .\n. . .\nCommands:\nclick 50 50\nclick 150 150\nwait "
                   "1000\nprint board") == "wR . .\n. . .\n");
    CHECK(runInput(" Board:\nwR wP . .\n. . . .\nCommands:\nclick 50 50\nclick 350 50\nwait "
                   "1000\nprint board") == "wR wP . .\n. . . .\n");
    CHECK(runInput(" Board:\nwR . . bP\nCommands:\nclick 50 50\nclick 350 50\nwait 3000\nprint "
                   "board") == ". . . wR\n");
    CHECK(runInput(" Board:\nwR . . wP\nCommands:\nclick 50 50\nclick 350 50\nwait 1000\nprint "
                   "board") == "wR . . wP\n");
}

TEST_CASE("Bishop movement") {
    CHECK(runInput(" Board:\nwB . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 250 250\nwait "
                   "2000\nprint board") == ". . .\n. . .\n. . wB\n");
    CHECK(runInput(" Board:\nwB . .\n. . .\nCommands:\nclick 50 50\nclick 250 50\nwait "
                   "1000\nprint board") == "wB . .\n. . .\n");
    CHECK(runInput(" Board:\nwB . .\n. wP .\n. . .\nCommands:\nclick 50 50\nclick 250 250\nwait "
                   "1000\nprint board") == "wB . .\n. wP .\n. . .\n");
}

TEST_CASE("Queen movement") {
    CHECK(runInput(" Board:\nwQ . . .\n. . . .\nCommands:\nclick 50 50\nclick 350 50\nwait "
                   "3000\nprint board") == ". . . wQ\n. . . .\n");
    CHECK(runInput(" Board:\nwQ . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 150 250\nwait "
                   "1000\nprint board") == "wQ . .\n. . .\n. . .\n");
}

TEST_CASE("Knight movement") {
    CHECK(runInput(" Board:\nwN . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 150 250\nwait "
                   "2000\nprint board") == ". . .\n. . .\n. wN .\n");
    CHECK(runInput(" Board:\nwN . .\n. . .\nCommands:\nclick 50 50\nclick 250 50\nwait "
                   "1000\nprint board") == "wN . .\n. . .\n");
    CHECK(runInput(" Board:\nwN . wP .\n. . . .\n. . . .\nCommands:\nclick 50 50\nclick 150 "
                   "250\nwait 2000\nprint board") == ". . wP .\n. . . .\n. wN . .\n");
    CHECK(runInput(" Board:\nwN . .\n. . bP\n. . .\nCommands:\nclick 50 50\nclick 250 150\nwait "
                   "2000\nprint board") == ". . .\n. . wN\n. . .\n");
}

TEST_CASE("Pawn movement") {
    CHECK(runInput(" Board:\n. . .\nwP . .\n. . .\nCommands:\nclick 50 150\nclick 50 50\nwait "
                   "1000\nprint board") == "wP . .\n. . .\n. . .\n");
    CHECK(runInput(" Board:\n. . .\n. . .\nwP . .\nCommands:\nclick 50 250\nclick 50 50\nwait "
                   "1000\nprint board") == ". . .\n. . .\nwP . .\n");
    CHECK(runInput(" Board:\nbP . .\nwP . .\n. . .\nCommands:\nclick 50 150\nclick 50 50\nwait "
                   "1000\nprint board") == "bP . .\nwP . .\n. . .\n");
    CHECK(runInput(" Board:\n. . .\nwP . .\n. . .\nCommands:\nclick 50 150\nclick 150 50\nwait "
                   "1000\nprint board") == ". . .\nwP . .\n. . .\n");
    CHECK(runInput(" Board:\n. bP .\nwP . .\n. . .\nCommands:\nclick 50 150\nclick 150 50\nwait "
                   "1000\nprint board") == ". wP .\n. . .\n. . .\n");
}

TEST_CASE("timed movement before and after arrival") {
    CHECK(runInput(" Board:\nwR . .\nCommands:\nclick 50 50\nclick 250 50\nwait 1000\nprint "
                   "board\nwait 1000\nprint board") == "wR . .\n. . wR\n");
    CHECK(runInput(" Board:\nwB . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 250 250\nwait 1000\n"
                   "print board\nwait 1000\nprint board") ==
          "wB . .\n. . .\n. . .\n. . .\n. . .\n. . wB\n");
}

TEST_CASE("architecture gap: enemy selection ignored") {
    CHECK(runInput(" Board:\nbK . wK\n. . .\nCommands:\nclick 50 50\nclick 150 150\nwait "
                   "1000\nprint board") == "bK . wK\n. . .\n");
}

TEST_CASE("architecture gap: concurrent pending moves") {
    CHECK(runInput(" Board:\nwR . .\nwK . .\nCommands:\nclick 50 50\nclick 250 50\nclick 50 "
                   "150\nclick 150 150\nwait 1000\nprint board\nwait 1000\nprint board") ==
           "wR . .\n. wK .\n. . wR\n. wK .\n");
}

TEST_CASE("architecture gap: print before wait shows source") {
    CHECK(runInput(" Board:\nwR . .\nCommands:\nclick 50 50\nclick 250 50\nprint board") ==
          "wR . .\n");
}

TEST_CASE("test_cannot_redirect_while_moving") {
    Board board = makeCommonRouteBoard();
    GameState state;
    state.reset();

    CHECK(movementStateAt(board, kSquareA_R, kSquareA_C) == PieceMovementState::Idle);

    clickSquare(state, board, kSquareA_R, kSquareA_C);
    clickSquare(state, board, kSquareB_R, kSquareB_C);

    CHECK(isPieceMovingAt(board, kSquareA_R, kSquareA_C));
    CHECK(pieceDestinationRow(board, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(board, kSquareA_R, kSquareA_C) == kSquareB_C);

    state.advanceTime(kMoveAToBDurationMs / 2, board);

    CHECK(isPieceMovingAt(board, kSquareA_R, kSquareA_C));
    CHECK(pieceDestinationRow(board, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(board, kSquareA_R, kSquareA_C) == kSquareB_C);

    clickSquare(state, board, kSquareA_R, kSquareA_C);
    clickSquare(state, board, kSquareC_R, kSquareC_C);

    CHECK(pieceDestinationRow(board, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(board, kSquareA_R, kSquareA_C) == kSquareB_C);

    state.advanceTime(kMoveAToBDurationMs / 2, board);

    CHECK(hasRookAt(board, kSquareB_R, kSquareB_C));
    CHECK_FALSE(hasRookAt(board, kSquareC_R, kSquareC_C));
    CHECK(movementStateAt(board, kSquareB_R, kSquareB_C) == PieceMovementState::Idle);
}

TEST_CASE("test_cannot_send_duplicate_command_while_moving") {
    Board board = makeCommonRouteBoard();
    GameState state;
    state.reset();

    clickSquare(state, board, kSquareA_R, kSquareA_C);
    clickSquare(state, board, kSquareB_R, kSquareB_C);

    CHECK(isPieceMovingAt(board, kSquareA_R, kSquareA_C));
    CHECK(pieceDestinationRow(board, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(board, kSquareA_R, kSquareA_C) == kSquareB_C);

    state.advanceTime(kMoveAToBDurationMs / 2, board);

    clickSquare(state, board, kSquareA_R, kSquareA_C);
    clickSquare(state, board, kSquareB_R, kSquareB_C);

    CHECK(pieceDestinationRow(board, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(board, kSquareA_R, kSquareA_C) == kSquareB_C);

    state.advanceTime(kMoveAToBDurationMs / 2, board);

    CHECK(hasRookAt(board, kSquareB_R, kSquareB_C));
    CHECK_FALSE(hasRookAt(board, kSquareA_R, kSquareA_C));
}

TEST_CASE("test_can_move_immediately_upon_arrival_no_cooldown") {
    Board board = makeCommonRouteBoard();
    GameState state;
    state.reset();

    clickSquare(state, board, kSquareA_R, kSquareA_C);
    clickSquare(state, board, kSquareB_R, kSquareB_C);
    state.advanceTime(kMoveAToBDurationMs, board);

    CHECK(hasRookAt(board, kSquareB_R, kSquareB_C));
    CHECK(movementStateAt(board, kSquareB_R, kSquareB_C) == PieceMovementState::Idle);

    clickSquare(state, board, kSquareB_R, kSquareB_C);
    clickSquare(state, board, kSquareC_R, kSquareC_C);

    CHECK(isPieceMovingAt(board, kSquareB_R, kSquareB_C));
    CHECK(pieceDestinationRow(board, kSquareB_R, kSquareB_C) == kSquareC_R);
    CHECK(pieceDestinationCol(board, kSquareB_R, kSquareB_C) == kSquareC_C);

    state.advanceTime(kMoveBToCDurationMs, board);

    CHECK(hasRookAt(board, kSquareC_R, kSquareC_C));
    CHECK_FALSE(hasRookAt(board, kSquareB_R, kSquareB_C));
}

TEST_CASE("test_piece_state_transitions_correctly") {
    Board board = makeCommonRouteBoard();
    GameState state;
    state.reset();

    CHECK(movementStateAt(board, kSquareA_R, kSquareA_C) == PieceMovementState::Idle);
    CHECK_FALSE(isPieceMovingAt(board, kSquareA_R, kSquareA_C));

    clickSquare(state, board, kSquareA_R, kSquareA_C);
    clickSquare(state, board, kSquareB_R, kSquareB_C);

    CHECK(movementStateAt(board, kSquareA_R, kSquareA_C) == PieceMovementState::Moving);
    CHECK(isPieceMovingAt(board, kSquareA_R, kSquareA_C));
    CHECK(pieceDestinationRow(board, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(board, kSquareA_R, kSquareA_C) == kSquareB_C);

    state.advanceTime(kMoveAToBDurationMs - 1, board);
    CHECK(movementStateAt(board, kSquareA_R, kSquareA_C) == PieceMovementState::Moving);

    state.advanceTime(1, board);

    CHECK(hasRookAt(board, kSquareB_R, kSquareB_C));
    CHECK(movementStateAt(board, kSquareB_R, kSquareB_C) == PieceMovementState::Idle);
    CHECK_FALSE(isPieceMovingAt(board, kSquareB_R, kSquareB_C));
}
