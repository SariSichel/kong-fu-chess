#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <sstream>
#include <string>

#include <cstdint>
#include <cstdlib>
#include <vector>

#include "constants.h"
#include "engine/game_engine.h"
#include "input/board_mapper.h"
#include "input/controller.h"
#include "io/board_parser.h"
#include "io/board_printer.h"
#include "texttests/script_runner.h"

namespace {

using model::Board;
using model::Color;
using model::Piece;
using model::PieceType;
using model::Position;

enum class TestMovementState {
    Idle,
    Moving,
    Airborne
};

Position pos(int row, int col) {
    return Position{row, col};
}

io::ParseResult parseInput(const std::string& input, Board& board) {
    std::istringstream in(input);
    return io::BoardParser::parseFromInput(in, board);
}

std::string runInput(const std::string& input) {
    std::istringstream in(input);
    std::ostringstream out;
    texttests::ScriptRunner runner;
    runner.run(in, out);
    return out.str();
}

int cellTopLeftX(int col) {
    return GameConfig::kBoardOriginX + col * GameConfig::kClickCellSize;
}

int cellTopLeftY(int row) {
    return GameConfig::kBoardOriginY + row * GameConfig::kClickCellSize;
}

void clickSquare(engine::GameEngine& engine, input::Controller& controller, int row, int col) {
    float centerX = 0.0f;
    float centerY = 0.0f;
    input::BoardMapper::toPixelCenter(row, col, centerX, centerY);
    controller.handleClick(engine, static_cast<int>(centerX), static_cast<int>(centerY));
}

void jumpSquare(engine::GameEngine& engine, input::Controller& controller, int row, int col) {
    float centerX = 0.0f;
    float centerY = 0.0f;
    input::BoardMapper::toPixelCenter(row, col, centerX, centerY);
    controller.handleJump(engine, static_cast<int>(centerX), static_cast<int>(centerY));
}

void copyBoardIntoEngine(engine::GameEngine& engine, const Board& source) {
    engine.board().clear();
    for (size_t row = 0; row < source.rows(); ++row) {
        std::vector<Piece> rowPieces;
        rowPieces.reserve(source.cols());
        for (size_t col = 0; col < source.cols(); ++col) {
            rowPieces.push_back(source.cell(pos(static_cast<int>(row), static_cast<int>(col))));
        }
        engine.board().addRow(std::move(rowPieces));
    }
    engine.reset();
}

bool hasRookAt(const Board& board, int row, int col, Color color = Color::White) {
    const Piece& piece = board.cell(pos(row, col));
    return piece.type() == PieceType::Rook && piece.color() == color;
}

bool hasPieceAt(const Board& board, int row, int col, PieceType type, Color color) {
    const Piece& piece = board.cell(pos(row, col));
    return piece.type() == type && piece.color() == color;
}

std::int64_t moveDurationMs(int fromR, int fromC, int toR, int toC) {
    const int dr = std::abs(toR - fromR);
    const int dc = std::abs(toC - fromC);
    const int distance = std::max(dr, dc);
    return static_cast<std::int64_t>(distance) * GameConfig::kMoveDurationMs;
}

Board makeCommonRouteBoard() {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    return board;
}

Board makeEnemyCollisionBoard() {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty(), Piece::empty(), Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty(), Piece::empty(), Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty(), Piece::empty(),
                  Piece(PieceType::Rook, Color::Black)});
    return board;
}

Board makePremoveCancellationBoard() {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty(), Piece::empty(),
                  Piece(PieceType::King, Color::White)});
    board.addRow({Piece::empty(), Piece::empty(), Piece::empty(), Piece::empty()});
    return board;
}

Board makeFriendlyLandingBoard() {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece(PieceType::King, Color::White),
                  Piece::empty(), Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty(), Piece::empty(), Piece::empty()});
    return board;
}

Board makeArrivalOrderBoard() {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty(), Piece(PieceType::King, Color::White),
                  Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty(), Piece::empty(), Piece::empty()});
    return board;
}

Board makeBasicKingCaptureBoard() {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece(PieceType::King, Color::Black)});
    return board;
}

Board makeIgnoreMovesAfterGameOverBoard() {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece(PieceType::King, Color::Black)});
    board.addRow({Piece::empty(), Piece(PieceType::Knight, Color::White), Piece::empty(),
                  Piece::empty()});
    return board;
}

Board makeKingStepsAwayBoard() {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(),
                  Piece(PieceType::King, Color::Black), Piece::empty()});
    return board;
}

TestMovementState movementStateAt(const engine::GameEngine& engine, int row, int col) {
    const Position square = pos(row, col);
    if (!engine.isBusyAt(square)) {
        return TestMovementState::Idle;
    }

    Position destination;
    if (engine.getActiveMoveDestination(square, destination)) {
        return TestMovementState::Moving;
    }

    return TestMovementState::Airborne;
}

bool isPieceMovingAt(const engine::GameEngine& engine, int row, int col) {
    return movementStateAt(engine, row, col) == TestMovementState::Moving;
}

int pieceDestinationRow(const engine::GameEngine& engine, int row, int col) {
    Position destination;
    if (engine.getActiveMoveDestination(pos(row, col), destination)) {
        return destination.row;
    }
    return -1;
}

int pieceDestinationCol(const engine::GameEngine& engine, int row, int col) {
    Position destination;
    if (engine.getActiveMoveDestination(pos(row, col), destination)) {
        return destination.col;
    }
    return -1;
}

constexpr int kSquareA_R = 0;
constexpr int kSquareA_C = 0;
constexpr int kSquareB_R = 0;
constexpr int kSquareB_C = 2;
constexpr int kSquareC_R = 0;
constexpr int kSquareC_C = 3;

constexpr std::int64_t kMoveAToBDurationMs = 2 * GameConfig::kMoveDurationMs;
constexpr std::int64_t kMoveBToCDurationMs = 1 * GameConfig::kMoveDurationMs;

}  // namespace

TEST_CASE("board_mapper: maps window pixels to grid positions using board origin") {
    float centerX = 0.0f;
    float centerY = 0.0f;

    input::BoardMapper::toPixelCenter(0, 0, centerX, centerY);
    CHECK(input::BoardMapper::toPosition(static_cast<int>(centerX), static_cast<int>(centerY)) ==
          pos(0, 0));
    input::BoardMapper::toPixelCenter(0, 1, centerX, centerY);
    CHECK(input::BoardMapper::toPosition(static_cast<int>(centerX), static_cast<int>(centerY)) ==
          pos(0, 1));
    input::BoardMapper::toPixelCenter(1, 0, centerX, centerY);
    CHECK(input::BoardMapper::toPosition(static_cast<int>(centerX), static_cast<int>(centerY)) ==
          pos(1, 0));
    input::BoardMapper::toPixelCenter(3, 2, centerX, centerY);
    CHECK(input::BoardMapper::toPosition(static_cast<int>(centerX), static_cast<int>(centerY)) ==
          pos(3, 2));

    CHECK(input::BoardMapper::toPosition(cellTopLeftX(0), cellTopLeftY(0)) == pos(0, 0));
    CHECK(input::BoardMapper::toPosition(cellTopLeftX(0) + GameConfig::kClickCellSize - 1,
                                         cellTopLeftY(0) + GameConfig::kClickCellSize - 1) ==
          pos(0, 0));
    CHECK(input::BoardMapper::toPosition(cellTopLeftX(1), cellTopLeftY(0)) == pos(0, 1));
    CHECK(input::BoardMapper::toPosition(cellTopLeftX(0), cellTopLeftY(1)) == pos(1, 0));

    input::BoardMapper::toPixelCenter(0, 0, centerX, centerY);
    CHECK(input::BoardMapper::toPosition(-1, static_cast<int>(centerY)) == pos(-1, -1));
    CHECK(input::BoardMapper::toPosition(static_cast<int>(centerX), -1) == pos(-1, -1));
    CHECK(input::BoardMapper::toPosition(5, 5) == pos(-1, -1));
    CHECK(input::BoardMapper::toPosition(cellTopLeftX(0) - 1, cellTopLeftY(0)) == pos(-1, -1));
}

TEST_CASE("board parsing and printing") {
    Board board;

    CHECK(parseInput(" Board:\nwP . wK\nwP . wP\nCommands:", board) == io::ParseResult::OK);
    CHECK(board.rows() == 2);
    CHECK(board.cols() == 3);

    std::ostringstream out;
    io::BoardPrinter::print(board, out);
    CHECK(out.str() == "wP . wK\nwP . wP\n");
}

TEST_CASE("parse errors") {
    Board board;

    CHECK(parseInput("Board:\ninvalid\nCommands:", board) == io::ParseResult::ERROR_UNKNOWN_TOKEN);
    CHECK(parseInput("Board:\nwP .\nwP . wP\nCommands:", board) ==
          io::ParseResult::ERROR_ROW_WIDTH_MISMATCH);
}

TEST_CASE("basic click and command integration") {
    CHECK(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 61 64\nclick 161 164\nwait "
                   "1000\nprint board") == ". . .\n. wK .\n. . .\n");
    CHECK(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 161 164\nclick 261 164\nwait "
                   "1000\nprint board") == "wK . .\n. . .\n. . .\n");
    CHECK(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 361 64\nclick 1 64\nprint "
                   "board") == "wK . .\n. . .\n. . .\n");
    CHECK(runInput(" Board:\nwR . wK\n. . .\nCommands:\nclick 61 64\nclick 261 64\nclick 261 "
                   "164\nwait 1000\nprint board") == "wR . .\n. . wK\n");
    CHECK(runInput(" Board:\nwK xZ\n. .\nCommands:\nprint board") == "ERROR UNKNOWN_TOKEN");
    CHECK(runInput(" Board:\nwK . .\n. bK\nCommands:\nprint board") ==
          "ERROR ROW_WIDTH_MISMATCH");
}

TEST_CASE("King movement") {
    CHECK(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 61 64\nclick 161 64\nwait "
                   "1000\nprint board") == ". wK .\n. . .\n. . .\n");
    CHECK(runInput(" Board:\nwK . .\n. . .\n. . .\nCommands:\nclick 61 64\nclick 261 64\nwait "
                   "1000\nprint board") == "wK . .\n. . .\n. . .\n");
}

TEST_CASE("Rook movement") {
    CHECK(runInput(" Board:\nwR . . .\n. . . .\nCommands:\nclick 61 64\nclick 361 64\nwait "
                   "3000\nprint board") == ". . . wR\n. . . .\n");
    CHECK(runInput(" Board:\nwR . .\n. . .\nCommands:\nclick 61 64\nclick 161 164\nwait "
                   "1000\nprint board") == "wR . .\n. . .\n");
    CHECK(runInput(" Board:\nwR wP . .\n. . . .\nCommands:\nclick 61 64\nclick 361 64\nwait "
                   "1000\nprint board") == "wR wP . .\n. . . .\n");
    CHECK(runInput(" Board:\nwR . . bP\nCommands:\nclick 61 64\nclick 361 64\nwait 3000\nprint "
                   "board") == ". . . wR\n");
    CHECK(runInput(" Board:\nwR . . wP\nCommands:\nclick 61 64\nclick 361 64\nwait 1000\nprint "
                   "board") == "wR . . wP\n");
}

TEST_CASE("Bishop movement") {
    CHECK(runInput(" Board:\nwB . .\n. . .\n. . .\nCommands:\nclick 61 64\nclick 261 264\nwait "
                   "2000\nprint board") == ". . .\n. . .\n. . wB\n");
    CHECK(runInput(" Board:\nwB . .\n. . .\nCommands:\nclick 61 64\nclick 261 64\nwait "
                   "1000\nprint board") == "wB . .\n. . .\n");
    CHECK(runInput(" Board:\nwB . .\n. wP .\n. . .\nCommands:\nclick 61 64\nclick 261 264\nwait "
                   "1000\nprint board") == "wB . .\n. wP .\n. . .\n");
}

TEST_CASE("Queen movement") {
    CHECK(runInput(" Board:\nwQ . . .\n. . . .\nCommands:\nclick 61 64\nclick 361 64\nwait "
                   "3000\nprint board") == ". . . wQ\n. . . .\n");
    CHECK(runInput(" Board:\nwQ . .\n. . .\n. . .\nCommands:\nclick 61 64\nclick 161 264\nwait "
                   "1000\nprint board") == "wQ . .\n. . .\n. . .\n");
}

TEST_CASE("Knight movement") {
    CHECK(runInput(" Board:\nwN . .\n. . .\n. . .\nCommands:\nclick 61 64\nclick 161 264\nwait "
                   "2000\nprint board") == ". . .\n. . .\n. wN .\n");
    CHECK(runInput(" Board:\nwN . .\n. . .\nCommands:\nclick 61 64\nclick 261 64\nwait "
                   "1000\nprint board") == "wN . .\n. . .\n");
    CHECK(runInput(" Board:\nwN . wP .\n. . . .\n. . . .\nCommands:\nclick 61 64\nclick 161 "
                   "264\nwait 2000\nprint board") == ". . wP .\n. . . .\n. wN . .\n");
    CHECK(runInput(" Board:\nwN . .\n. . bP\n. . .\nCommands:\nclick 61 64\nclick 261 164\nwait "
                   "2000\nprint board") == ". . .\n. . wN\n. . .\n");
}

TEST_CASE("Pawn movement") {
    CHECK(runInput(" Board:\n. . .\nwP . .\n. . .\nCommands:\nclick 61 164\nclick 61 64\nwait "
                   "1000\nprint board") == "wQ . .\n. . .\n. . .\n");
    CHECK(runInput(" Board:\n. . .\n. . .\nwP . .\nCommands:\nclick 61 264\nclick 61 64\nwait "
                   "1000\nprint board") == ". . .\n. . .\nwP . .\n");
    CHECK(runInput(" Board:\nbP . .\nwP . .\n. . .\nCommands:\nclick 61 164\nclick 61 64\nwait "
                   "1000\nprint board") == "bP . .\nwP . .\n. . .\n");
    CHECK(runInput(" Board:\n. . .\nwP . .\n. . .\nCommands:\nclick 61 164\nclick 161 64\nwait "
                   "1000\nprint board") == ". . .\nwP . .\n. . .\n");
    CHECK(runInput(" Board:\n. bP .\nwP . .\n. . .\nCommands:\nclick 61 164\nclick 161 64\nwait "
                   "1000\nprint board") == ". wQ .\n. . .\n. . .\n");
}

// --- Pawn advanced rules: double move, path clearance, promotion ---

TEST_CASE("pawn: valid single step forward (white, no promotion)") {
    // 3-row board => white double-step start row is rows()-2 == 1. A single step is always legal;
    // the destination (row 1) is not the last row, so no promotion happens.
    CHECK(runInput(" Board:\n. .\n. .\nwP .\nCommands:\nclick 61 264\nclick 61 164\nwait "
                   "1000\nprint board") == ". .\nwP .\n. .\n");
}

TEST_CASE("pawn: valid single step forward (black, no promotion)") {
    // Black moves down (increasing row). Pawn at row 0 steps to row 1 (not last row).
    CHECK(runInput(" Board:\nbP .\n. .\n. .\nCommands:\nclick 61 64\nclick 61 164\nwait "
                   "1000\nprint board") == ". .\nbP .\n. .\n");
}

TEST_CASE("pawn: valid double step from start row (white)") {
    // 5-row board => white double-step start row is rows()-2 == 3. Pawn jumps 3 -> 1.
    CHECK(runInput(" Board:\n. .\n. .\n. .\nwP .\n. .\nCommands:\nclick 61 364\nclick 61 "
                   "164\nwait 2000\nprint board") == ". .\nwP .\n. .\n. .\n. .\n");
}

TEST_CASE("pawn: valid double step from start row (black)") {
    // 5-row board => black double-step start row is 1. Pawn jumps 1 -> 3.
    CHECK(runInput(" Board:\n. .\nbP .\n. .\n. .\n. .\nCommands:\nclick 61 164\nclick 61 "
                   "364\nwait 2000\nprint board") == ". .\n. .\n. .\nbP .\n. .\n");
}

TEST_CASE("pawn: double step blocked by intermediate cell is rejected") {
    // White pawn on the start row (row 3). Intermediate cell (row 2) is occupied, so
    // the 3 -> 1 double step must be rejected and nothing on the board changes.
    CHECK(runInput(" Board:\n. .\n. .\n. .\nbP .\nwP .\nCommands:\nclick 61 364\nclick 61 "
                   "164\nwait 2000\nprint board") == ". .\n. .\n. .\nbP .\nwP .\n");
}

TEST_CASE("pawn: double step blocked by occupied target is rejected") {
    // White pawn on the start row (row 3). Target cell (row 1) is occupied, so the
    // 3 -> 1 double step must be rejected and nothing on the board changes.
    CHECK(runInput(" Board:\n. .\n. .\nbP .\n. .\nwP .\nCommands:\nclick 61 364\nclick 61 "
                   "164\nwait 2000\nprint board") == ". .\n. .\nbP .\n. .\nwP .\n");
}

TEST_CASE("pawn: double step from a non-start row is rejected") {
    // 5-row board => white double-step start row is 3. Pawn sits on row 2, so a
    // 2 -> 0 double step is illegal and the pawn stays put.
    CHECK(runInput(" Board:\n. .\n. .\nwP .\n. .\n. .\nCommands:\nclick 61 264\nclick 61 "
                   "64\nwait 2000\nprint board") == ". .\n. .\nwP .\n. .\n. .\n");
}

TEST_CASE("pawn: single-step promotion into final row (white)") {
    // White pawn one step from the top row (row 0) promotes to a Queen on arrival.
    CHECK(runInput(" Board:\n. .\nwP .\nCommands:\nclick 61 164\nclick 61 64\nwait "
                   "1000\nprint board") == "wQ .\n. .\n");
}

TEST_CASE("pawn: single-step promotion into final row (black)") {
    // Black pawn one step from the bottom row (last row) promotes to a Queen.
    CHECK(runInput(" Board:\nbP .\n. .\nCommands:\nclick 61 64\nclick 61 164\nwait "
                   "1000\nprint board") == ". .\nbQ .\n");
}

TEST_CASE("pawn: double-step promotion into final row (white)") {
    // 4-row 2-col board uses standard mapping. White double-step start row is rows()-2 == 2.
    CHECK(runInput(" Board:\n. .\n. .\nwP .\n. .\nCommands:\nclick 61 264\nclick 61 64\nwait "
                   "2000\nprint board") == "wQ .\n. .\n. .\n. .\n");
}

TEST_CASE("pawn: double-step promotion into final row (black)") {
    // 4-row 2-col board uses standard mapping. Black double-step start row is 1.
    CHECK(runInput(" Board:\n. .\nbP .\n. .\n. .\nCommands:\nclick 61 164\nclick 61 364\nwait "
                   "2000\nprint board") == ". .\n. .\n. .\nbQ .\n");
}

TEST_CASE("pawn: VPL white double from start on 4x3 board") {
    CHECK(runInput(" Board:\n. . .\n. . .\n. wP .\n. . .\nCommands:\nclick 161 264\nclick 161 "
                   "64\nwait 2000\nprint board") == ". wQ .\n. . .\n. . .\n. . .\n");
}

TEST_CASE("pawn: VPL black double from start on 4x3 board") {
    CHECK(runInput(" Board:\n. . .\n. bP .\n. . .\n. . .\nCommands:\nclick 161 164\nclick 161 "
                   "364\nwait 2000\nprint board") == ". . .\n. . .\n. . .\n. bQ .\n");
}

TEST_CASE("pawn: promotion happens via GameState arrival (state-level check)") {
    // Verify at the object level that validation runs before any board mutation and
    // that the pawn becomes a Queen only once it settles on the last row.
    Board board;
    board.addRow({Piece::empty(), Piece::empty()});
    board.addRow({Piece(PieceType::Pawn, Color::White), Piece::empty()});

    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    clickSquare(engine, controller, 1, 0);
    clickSquare(engine, controller, 0, 0);

    // Move is validated and started, but the piece is still a Pawn mid-flight.
    CHECK(isPieceMovingAt(engine, 1, 0));
    CHECK(engine.board().cell(pos(1, 0)).type() == PieceType::Pawn);

    engine.advanceTime(static_cast<int>(moveDurationMs(1, 0, 0, 0)));

    CHECK(hasPieceAt(engine.board(), 0, 0, PieceType::Queen, Color::White));
    CHECK_FALSE(hasPieceAt(engine.board(), 0, 0, PieceType::Pawn, Color::White));
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Idle);
}

TEST_CASE("pawn: invalid double step leaves the board completely unchanged") {
    // Strict validation ordering: an illegal move must alter nothing on the board.
    Board board;
    board.addRow({Piece::empty(), Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty()});
    board.addRow({Piece(PieceType::Pawn, Color::White), Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty()});

    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    // Pawn at row 2 is NOT on the double-step start row (rows()-2 == 3), so a 2 -> 0 jump is invalid.
    clickSquare(engine, controller, 2, 0);
    clickSquare(engine, controller, 0, 0);

    CHECK_FALSE(isPieceMovingAt(engine, 2, 0));
    CHECK(hasPieceAt(engine.board(), 2, 0, PieceType::Pawn, Color::White));

    engine.advanceTime(static_cast<int>(moveDurationMs(2, 0, 0, 0)));

    CHECK(hasPieceAt(engine.board(), 2, 0, PieceType::Pawn, Color::White));
    CHECK_FALSE(hasPieceAt(engine.board(), 0, 0, PieceType::Pawn, Color::White));
    CHECK_FALSE(hasPieceAt(engine.board(), 0, 0, PieceType::Queen, Color::White));
}

TEST_CASE("timed movement before and after arrival") {
    CHECK(runInput(" Board:\nwR . .\nCommands:\nclick 61 64\nclick 261 64\nwait 1000\nprint "
                   "board\nwait 1000\nprint board") == "wR . .\n. . wR\n");
    CHECK(runInput(" Board:\nwB . .\n. . .\n. . .\nCommands:\nclick 61 64\nclick 261 264\nwait 1000\n"
                   "print board\nwait 1000\nprint board") ==
          "wB . .\n. . .\n. . .\n. . .\n. . .\n. . wB\n");
}

TEST_CASE("enemy collision via click integration") {
    CHECK(runInput(" Board:\nwR . . bR\nCommands:\nclick 361 64\nclick 61 64\nclick 61 64\nclick "
                   "361 64\nwait 3000\nprint board") == "bR . . .\n");
}

TEST_CASE("architecture gap: concurrent pending moves") {
    CHECK(runInput(" Board:\nwR . .\nwK . .\nCommands:\nclick 61 64\nclick 261 64\nclick 61 "
                   "164\nclick 161 164\nwait 1000\nprint board\nwait 1000\nprint board") ==
           "wR . .\n. wK .\n. . wR\n. wK .\n");
}

TEST_CASE("architecture gap: print before wait shows source") {
    CHECK(runInput(" Board:\nwR . .\nCommands:\nclick 61 64\nclick 261 164\nprint board") ==
          "wR . .\n");
}

TEST_CASE("test_cannot_redirect_while_moving") {
    Board board = makeCommonRouteBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    CHECK(movementStateAt(engine, kSquareA_R, kSquareA_C) == TestMovementState::Idle);

    clickSquare(engine, controller, kSquareA_R, kSquareA_C);
    clickSquare(engine, controller, kSquareB_R, kSquareB_C);

    CHECK(isPieceMovingAt(engine, kSquareA_R, kSquareA_C));
    CHECK(pieceDestinationRow(engine, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(engine, kSquareA_R, kSquareA_C) == kSquareB_C);

    engine.advanceTime(static_cast<int>(kMoveAToBDurationMs / 2));

    CHECK(isPieceMovingAt(engine, kSquareA_R, kSquareA_C));
    CHECK(pieceDestinationRow(engine, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(engine, kSquareA_R, kSquareA_C) == kSquareB_C);

    clickSquare(engine, controller, kSquareA_R, kSquareA_C);
    clickSquare(engine, controller, kSquareC_R, kSquareC_C);

    CHECK(pieceDestinationRow(engine, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(engine, kSquareA_R, kSquareA_C) == kSquareB_C);

    engine.advanceTime(static_cast<int>(kMoveAToBDurationMs / 2));

    CHECK(hasRookAt(engine.board(), kSquareB_R, kSquareB_C));
    CHECK_FALSE(hasRookAt(engine.board(), kSquareC_R, kSquareC_C));
    CHECK(movementStateAt(engine, kSquareB_R, kSquareB_C) == TestMovementState::Idle);
}

TEST_CASE("test_cannot_send_duplicate_command_while_moving") {
    Board board = makeCommonRouteBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    clickSquare(engine, controller, kSquareA_R, kSquareA_C);
    clickSquare(engine, controller, kSquareB_R, kSquareB_C);

    CHECK(isPieceMovingAt(engine, kSquareA_R, kSquareA_C));
    CHECK(pieceDestinationRow(engine, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(engine, kSquareA_R, kSquareA_C) == kSquareB_C);

    engine.advanceTime(static_cast<int>(kMoveAToBDurationMs / 2));

    clickSquare(engine, controller, kSquareA_R, kSquareA_C);
    clickSquare(engine, controller, kSquareB_R, kSquareB_C);

    CHECK(pieceDestinationRow(engine, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(engine, kSquareA_R, kSquareA_C) == kSquareB_C);

    engine.advanceTime(static_cast<int>(kMoveAToBDurationMs / 2));

    CHECK(hasRookAt(engine.board(), kSquareB_R, kSquareB_C));
    CHECK_FALSE(hasRookAt(engine.board(), kSquareA_R, kSquareA_C));
}

TEST_CASE("test_can_move_immediately_upon_arrival_no_cooldown") {
    Board board = makeCommonRouteBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    clickSquare(engine, controller, kSquareA_R, kSquareA_C);
    clickSquare(engine, controller, kSquareB_R, kSquareB_C);
    engine.advanceTime(static_cast<int>(kMoveAToBDurationMs));

    CHECK(hasRookAt(engine.board(), kSquareB_R, kSquareB_C));
    CHECK(movementStateAt(engine, kSquareB_R, kSquareB_C) == TestMovementState::Idle);

    clickSquare(engine, controller, kSquareB_R, kSquareB_C);
    clickSquare(engine, controller, kSquareC_R, kSquareC_C);

    CHECK(isPieceMovingAt(engine, kSquareB_R, kSquareB_C));
    CHECK(pieceDestinationRow(engine, kSquareB_R, kSquareB_C) == kSquareC_R);
    CHECK(pieceDestinationCol(engine, kSquareB_R, kSquareB_C) == kSquareC_C);

    engine.advanceTime(static_cast<int>(kMoveBToCDurationMs));

    CHECK(hasRookAt(engine.board(), kSquareC_R, kSquareC_C));
    CHECK_FALSE(hasRookAt(engine.board(), kSquareB_R, kSquareB_C));
}

TEST_CASE("test_piece_state_transitions_correctly") {
    Board board = makeCommonRouteBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    CHECK(movementStateAt(engine, kSquareA_R, kSquareA_C) == TestMovementState::Idle);
    CHECK_FALSE(isPieceMovingAt(engine, kSquareA_R, kSquareA_C));

    clickSquare(engine, controller, kSquareA_R, kSquareA_C);
    clickSquare(engine, controller, kSquareB_R, kSquareB_C);

    CHECK(movementStateAt(engine, kSquareA_R, kSquareA_C) == TestMovementState::Moving);
    CHECK(isPieceMovingAt(engine, kSquareA_R, kSquareA_C));
    CHECK(pieceDestinationRow(engine, kSquareA_R, kSquareA_C) == kSquareB_R);
    CHECK(pieceDestinationCol(engine, kSquareA_R, kSquareA_C) == kSquareB_C);

    engine.advanceTime(static_cast<int>(kMoveAToBDurationMs - 1));
    CHECK(movementStateAt(engine, kSquareA_R, kSquareA_C) == TestMovementState::Moving);

    engine.advanceTime(static_cast<int>(1));

    CHECK(hasRookAt(engine.board(), kSquareB_R, kSquareB_C));
    CHECK(movementStateAt(engine, kSquareB_R, kSquareB_C) == TestMovementState::Idle);
    CHECK_FALSE(isPieceMovingAt(engine, kSquareB_R, kSquareB_C));
}

// --- Advanced real-time interaction tests ---

TEST_CASE("test_enemy_collision_black_started_first") {
    Board board = makeEnemyCollisionBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kWhiteR = 0;
    constexpr int kWhiteC = 0;
    constexpr int kBlackR = 3;
    constexpr int kBlackC = 3;
    constexpr int kDestR = 0;
    constexpr int kDestC = 3;
    const std::int64_t kTravelMs = moveDurationMs(kWhiteR, kWhiteC, kDestR, kDestC);

    clickSquare(engine, controller, kBlackR, kBlackC);
    clickSquare(engine, controller, kDestR, kDestC);
    engine.advanceTime(static_cast<int>(500));
    clickSquare(engine, controller, kWhiteR, kWhiteC);
    clickSquare(engine, controller, kDestR, kDestC);

    engine.advanceTime(static_cast<int>(kTravelMs));
    engine.advanceTime(static_cast<int>(500));

    CHECK(hasRookAt(engine.board(), kDestR, kDestC, Color::Black));
    CHECK_FALSE(hasRookAt(engine.board(), kWhiteR, kWhiteC, Color::White));
    CHECK_FALSE(hasRookAt(engine.board(), kDestR, kDestC, Color::White));
}

TEST_CASE("test_enemy_collision_white_started_first") {
    Board board = makeEnemyCollisionBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kWhiteR = 0;
    constexpr int kWhiteC = 0;
    constexpr int kBlackR = 3;
    constexpr int kBlackC = 3;
    constexpr int kDestR = 0;
    constexpr int kDestC = 3;
    const std::int64_t kTravelMs = moveDurationMs(kWhiteR, kWhiteC, kDestR, kDestC);

    clickSquare(engine, controller, kWhiteR, kWhiteC);
    clickSquare(engine, controller, kDestR, kDestC);
    engine.advanceTime(static_cast<int>(500));
    clickSquare(engine, controller, kBlackR, kBlackC);
    clickSquare(engine, controller, kDestR, kDestC);

    engine.advanceTime(static_cast<int>(kTravelMs));
    engine.advanceTime(static_cast<int>(500));

    CHECK(hasRookAt(engine.board(), kDestR, kDestC, Color::White));
    CHECK_FALSE(hasRookAt(engine.board(), kBlackR, kBlackC, Color::Black));
    CHECK_FALSE(hasRookAt(engine.board(), kDestR, kDestC, Color::Black));
}

TEST_CASE("test_enemy_collision_absolute_tie") {
    Board board = makeEnemyCollisionBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kWhiteR = 0;
    constexpr int kWhiteC = 0;
    constexpr int kBlackR = 3;
    constexpr int kBlackC = 3;
    constexpr int kDestR = 0;
    constexpr int kDestC = 3;
    const std::int64_t kTravelMs = moveDurationMs(kWhiteR, kWhiteC, kDestR, kDestC);

    clickSquare(engine, controller, kBlackR, kBlackC);
    clickSquare(engine, controller, kDestR, kDestC);
    clickSquare(engine, controller, kWhiteR, kWhiteC);
    clickSquare(engine, controller, kDestR, kDestC);

    engine.advanceTime(static_cast<int>(kTravelMs));

    // Tie-breaker: earlier command order wins (black requested first at the same tick).
    CHECK(hasRookAt(engine.board(), kDestR, kDestC, Color::Black));
    CHECK_FALSE(hasRookAt(engine.board(), kDestR, kDestC, Color::White));
    CHECK_FALSE(hasRookAt(engine.board(), kWhiteR, kWhiteC, Color::White));
}

TEST_CASE("test_premove_executes_on_arrival") {
    Board board = makeCommonRouteBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    clickSquare(engine, controller, kSquareA_R, kSquareA_C);
    clickSquare(engine, controller, kSquareB_R, kSquareB_C);

    CHECK(isPieceMovingAt(engine, kSquareA_R, kSquareA_C));

    clickSquare(engine, controller, kSquareA_R, kSquareA_C);
    clickSquare(engine, controller, kSquareC_R, kSquareC_C);

    CHECK(engine.hasPremoveAt(pos(kSquareA_R, kSquareA_C)));

    engine.advanceTime(static_cast<int>(kMoveAToBDurationMs));

    CHECK(hasRookAt(engine.board(), kSquareB_R, kSquareB_C));
    CHECK(isPieceMovingAt(engine, kSquareB_R, kSquareB_C));
    CHECK(pieceDestinationRow(engine, kSquareB_R, kSquareB_C) == kSquareC_R);
    CHECK(pieceDestinationCol(engine, kSquareB_R, kSquareB_C) == kSquareC_C);

    engine.advanceTime(static_cast<int>(kMoveBToCDurationMs));

    CHECK(hasRookAt(engine.board(), kSquareC_R, kSquareC_C));
    CHECK(movementStateAt(engine, kSquareC_R, kSquareC_C) == TestMovementState::Idle);
}

TEST_CASE("test_premove_cancelled_when_target_blocked") {
    Board board = makePremoveCancellationBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int local_kSquareB_R = 0;
    constexpr int local_kSquareB_C = 2;
    constexpr int local_kSquareC_R = 0;
    constexpr int local_kSquareC_C = 3;
    constexpr int kKingR = 1;
    constexpr int kKingC = 3;
    const std::int64_t kRookToBMs =
        moveDurationMs(kRookR, kRookC, local_kSquareB_R, local_kSquareB_C);
    const std::int64_t kKingToCMs =
        moveDurationMs(kKingR, kKingC, local_kSquareC_R, local_kSquareC_C);

    clickSquare(engine, controller, kRookR, kRookC);
    clickSquare(engine, controller, local_kSquareB_R, local_kSquareB_C);

    clickSquare(engine, controller, kRookR, kRookC);
    clickSquare(engine, controller, local_kSquareC_R, local_kSquareC_C);

    CHECK(engine.hasPremoveAt(pos(kRookR, kRookC)));

    engine.advanceTime(static_cast<int>(500));
    clickSquare(engine, controller, kKingR, kKingC);
    clickSquare(engine, controller, local_kSquareC_R, local_kSquareC_C);

    engine.advanceTime(static_cast<int>(kKingToCMs));
    CHECK(hasPieceAt(engine.board(), local_kSquareC_R, local_kSquareC_C, PieceType::King,
                     Color::White));

    engine.advanceTime(static_cast<int>(kRookToBMs - 500));

    CHECK(hasRookAt(engine.board(), local_kSquareB_R, local_kSquareB_C));
    CHECK(movementStateAt(engine, local_kSquareB_R, local_kSquareB_C) == TestMovementState::Idle);
    CHECK_FALSE(isPieceMovingAt(engine, local_kSquareB_R, local_kSquareB_C));
    CHECK_FALSE(hasRookAt(engine.board(), local_kSquareC_R, local_kSquareC_C));
}

TEST_CASE("test_friendly_landing_blocked_at_arrival") {
    Board board = makeFriendlyLandingBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 1;
    constexpr int kTargetR = 0;
    constexpr int kTargetC = 2;
    const std::int64_t kRookTravelMs = moveDurationMs(kRookR, kRookC, kTargetR, kTargetC);
    const std::int64_t kKingTravelMs = moveDurationMs(kKingR, kKingC, kTargetR, kTargetC);

    clickSquare(engine, controller, kRookR, kRookC);
    clickSquare(engine, controller, kTargetR, kTargetC);
    clickSquare(engine, controller, kKingR, kKingC);
    clickSquare(engine, controller, kTargetR, kTargetC);

    engine.advanceTime(static_cast<int>(kKingTravelMs));
    CHECK(hasPieceAt(engine.board(), kTargetR, kTargetC, PieceType::King, Color::White));

    engine.advanceTime(static_cast<int>(kRookTravelMs - kKingTravelMs));

    CHECK(hasRookAt(engine.board(), kRookR, kRookC));
    CHECK(movementStateAt(engine, kRookR, kRookC) == TestMovementState::Idle);
    CHECK(hasPieceAt(engine.board(), kTargetR, kTargetC, PieceType::King, Color::White));
    CHECK_FALSE(hasRookAt(engine.board(), kTargetR, kTargetC));
}

TEST_CASE("test_arrival_processing_order_by_started_at") {
    Board board = makeArrivalOrderBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 1;
    constexpr int kKingC = 2;
    constexpr int kTargetR = 0;
    constexpr int kTargetC = 2;
    const std::int64_t kRookTravelMs = moveDurationMs(kRookR, kRookC, kTargetR, kTargetC);
    const std::int64_t kKingTravelMs = moveDurationMs(kKingR, kKingC, kTargetR, kTargetC);

    clickSquare(engine, controller, kRookR, kRookC);
    clickSquare(engine, controller, kTargetR, kTargetC);
    engine.advanceTime(static_cast<int>(1000));
    clickSquare(engine, controller, kKingR, kKingC);
    clickSquare(engine, controller, kTargetR, kTargetC);

    CHECK(kRookTravelMs == kKingTravelMs + 1000);

    engine.advanceTime(static_cast<int>(kKingTravelMs));

    CHECK(hasRookAt(engine.board(), kTargetR, kTargetC));
    CHECK(hasPieceAt(engine.board(), kKingR, kKingC, PieceType::King, Color::White));
    CHECK(movementStateAt(engine, kKingR, kKingC) == TestMovementState::Idle);
    CHECK_FALSE(hasPieceAt(engine.board(), kTargetR, kTargetC, PieceType::King, Color::White));
}

// --- Game-over behavior tests (TDD: requires GameState::isGameOver()) ---

TEST_CASE("game over: basic king capture ends the game") {
    Board board = makeBasicKingCaptureBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 3;
    const std::int64_t kCaptureMs = moveDurationMs(kRookR, kRookC, kKingR, kKingC);

    CHECK_FALSE(engine.isGameOver());

    clickSquare(engine, controller, kRookR, kRookC);
    clickSquare(engine, controller, kKingR, kKingC);

    CHECK_FALSE(engine.isGameOver());
    CHECK(isPieceMovingAt(engine, kRookR, kRookC));

    engine.advanceTime(static_cast<int>(kCaptureMs));

    CHECK(engine.isGameOver());
    CHECK(hasRookAt(engine.board(), kKingR, kKingC, Color::White));
    CHECK_FALSE(hasPieceAt(engine.board(), kKingR, kKingC, PieceType::King, Color::Black));
}

TEST_CASE("game over: moves are ignored after king capture") {
    Board board = makeIgnoreMovesAfterGameOverBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 3;
    constexpr int kKnightR = 1;
    constexpr int kKnightC = 1;
    constexpr int kKnightDestR = 1;
    constexpr int kKnightDestC = 2;
    const std::int64_t kCaptureMs = moveDurationMs(kRookR, kRookC, kKingR, kKingC);

    clickSquare(engine, controller, kRookR, kRookC);
    clickSquare(engine, controller, kKingR, kKingC);
    engine.advanceTime(static_cast<int>(kCaptureMs));

    CHECK(engine.isGameOver());
    CHECK(hasRookAt(engine.board(), kKingR, kKingC, Color::White));
    CHECK(hasPieceAt(engine.board(), kKnightR, kKnightC, PieceType::Knight, Color::White));

    clickSquare(engine, controller, kKnightR, kKnightC);
    clickSquare(engine, controller, kKnightDestR, kKnightDestC);

    CHECK(hasPieceAt(engine.board(), kKnightR, kKnightC, PieceType::Knight, Color::White));
    CHECK_FALSE(isPieceMovingAt(engine, kKnightR, kKnightC));
    CHECK_FALSE(hasPieceAt(engine.board(), kKnightDestR, kKnightDestC, PieceType::Knight, Color::White));

    engine.advanceTime(static_cast<int>(
        moveDurationMs(kKnightR, kKnightC, kKnightDestR, kKnightDestC)));

    CHECK(hasPieceAt(engine.board(), kKnightR, kKnightC, PieceType::Knight, Color::White));
    CHECK_FALSE(hasPieceAt(engine.board(), kKnightDestR, kKnightDestC, PieceType::Knight, Color::White));
    CHECK(engine.isGameOver());
}

TEST_CASE("game over: friendly collision on own king does not end game") {
    Board board = makeFriendlyLandingBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 1;
    constexpr int kTargetR = 0;
    constexpr int kTargetC = 2;
    const std::int64_t kRookTravelMs = moveDurationMs(kRookR, kRookC, kTargetR, kTargetC);
    const std::int64_t kKingTravelMs = moveDurationMs(kKingR, kKingC, kTargetR, kTargetC);

    CHECK_FALSE(engine.isGameOver());

    clickSquare(engine, controller, kRookR, kRookC);
    clickSquare(engine, controller, kTargetR, kTargetC);
    clickSquare(engine, controller, kKingR, kKingC);
    clickSquare(engine, controller, kTargetR, kTargetC);

    engine.advanceTime(static_cast<int>(kKingTravelMs));
    CHECK(hasPieceAt(engine.board(), kTargetR, kTargetC, PieceType::King, Color::White));

    engine.advanceTime(static_cast<int>(kRookTravelMs - kKingTravelMs));

    CHECK_FALSE(engine.isGameOver());
    CHECK(hasRookAt(engine.board(), kRookR, kRookC));
    CHECK(hasPieceAt(engine.board(), kTargetR, kTargetC, PieceType::King, Color::White));
    CHECK_FALSE(hasRookAt(engine.board(), kTargetR, kTargetC));
}

TEST_CASE("game over: simultaneous arrival tie does not capture the king") {
    Board board = makeArrivalOrderBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 1;
    constexpr int kKingC = 2;
    constexpr int kTargetR = 0;
    constexpr int kTargetC = 2;
    const std::int64_t kRookTravelMs = moveDurationMs(kRookR, kRookC, kTargetR, kTargetC);
    const std::int64_t kKingTravelMs = moveDurationMs(kKingR, kKingC, kTargetR, kTargetC);

    CHECK_FALSE(engine.isGameOver());

    // Rook starts first; king races to the same square and arrives on the same tick.
    clickSquare(engine, controller, kRookR, kRookC);
    clickSquare(engine, controller, kTargetR, kTargetC);
    engine.advanceTime(static_cast<int>(1000));
    clickSquare(engine, controller, kKingR, kKingC);
    clickSquare(engine, controller, kTargetR, kTargetC);

    CHECK(kRookTravelMs == kKingTravelMs + 1000);

    engine.advanceTime(static_cast<int>(kKingTravelMs));

    // Tie-breaker: rook started first, so it occupies the square. The king is not captured.
    CHECK_FALSE(engine.isGameOver());
    CHECK(hasRookAt(engine.board(), kTargetR, kTargetC));
    CHECK(hasPieceAt(engine.board(), kKingR, kKingC, PieceType::King, Color::White));
    CHECK(movementStateAt(engine, kKingR, kKingC) == TestMovementState::Idle);
    CHECK_FALSE(hasPieceAt(engine.board(), kTargetR, kTargetC, PieceType::King, Color::White));
}

TEST_CASE("game over: king steps away before attacker arrives") {
    Board board = makeKingStepsAwayBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 2;
    constexpr int kKingDestR = 0;
    constexpr int kKingDestC = 3;
    const std::int64_t kRookTravelMs = moveDurationMs(kRookR, kRookC, kKingR, kKingC);
    const std::int64_t kKingTravelMs = moveDurationMs(kKingR, kKingC, kKingDestR, kKingDestC);

    CHECK_FALSE(engine.isGameOver());

    clickSquare(engine, controller, kRookR, kRookC);
    clickSquare(engine, controller, kKingR, kKingC);
    clickSquare(engine, controller, kKingR, kKingC);
    clickSquare(engine, controller, kKingDestR, kKingDestC);

    engine.advanceTime(static_cast<int>(kKingTravelMs));

    CHECK(hasPieceAt(engine.board(), kKingDestR, kKingDestC, PieceType::King, Color::Black));
    CHECK_FALSE(hasPieceAt(engine.board(), kKingR, kKingC, PieceType::King, Color::Black));
    CHECK(isPieceMovingAt(engine, kRookR, kRookC));

    engine.advanceTime(static_cast<int>(kRookTravelMs - kKingTravelMs));

    CHECK_FALSE(engine.isGameOver());
    CHECK(hasRookAt(engine.board(), kKingR, kKingC, Color::White));
    CHECK(hasPieceAt(engine.board(), kKingDestR, kKingDestC, PieceType::King, Color::Black));
    CHECK_FALSE(hasPieceAt(engine.board(), kKingR, kKingC, PieceType::King, Color::Black));
}

TEST_CASE("game over: reset clears game-over flag") {
    Board board = makeBasicKingCaptureBoard();
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 3;
    const std::int64_t kCaptureMs = moveDurationMs(kRookR, kRookC, kKingR, kKingC);

    clickSquare(engine, controller, kRookR, kRookC);
    clickSquare(engine, controller, kKingR, kKingC);
    engine.advanceTime(static_cast<int>(kCaptureMs));

    CHECK(engine.isGameOver());

    engine.reset();

    CHECK_FALSE(engine.isGameOver());
}

// --- Jump ("Airborne") mechanic tests ---

TEST_CASE("jump: normal landing returns to idle on the same cell") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    jumpSquare(engine, controller, 0, 0);
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Airborne);

    // Still airborne one tick before the timer expires.
    engine.advanceTime(static_cast<int>(GameConfig::kJumpDurationMs - 1));
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Airborne);

    engine.advanceTime(static_cast<int>(1));
    CHECK(hasRookAt(engine.board(), 0, 0));
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Idle);
}

TEST_CASE("jump: airborne piece captures an arriving enemy and stays put") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece(PieceType::Rook, Color::Black)});
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    // Black rook starts at t=0 heading for (0,0); a 3-cell trip arrives at t=3000.
    clickSquare(engine, controller, 0, 3);
    clickSquare(engine, controller, 0, 0);
    CHECK(isPieceMovingAt(engine, 0, 3));

    // Jump at t=2500 so the white rook is airborne across the 3000ms arrival.
    engine.advanceTime(static_cast<int>(2500));
    jumpSquare(engine, controller, 0, 0);
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Airborne);

    // At t=3000 the enemy arrives while the defender is still airborne.
    engine.advanceTime(static_cast<int>(500));
    CHECK(hasRookAt(engine.board(), 0, 0, Color::White));
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Airborne);
    CHECK_FALSE(hasRookAt(engine.board(), 0, 3, Color::Black));
    CHECK(engine.board().cell(pos(0, 3)).isEmpty());

    // The jump keeps running to its full duration, then lands in place.
    engine.advanceTime(static_cast<int>(500));
    CHECK(hasRookAt(engine.board(), 0, 0, Color::White));
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Idle);
}

TEST_CASE("jump: friendly move in flight toward an airborne ally is cancelled") {
    // Layout: . wK . wR  -> king at (0,1), rook at (0,3), target square X = (0,2).
    Board board;
    board.addRow({Piece::empty(), Piece(PieceType::King, Color::White), Piece::empty(),
                  Piece(PieceType::Rook, Color::White)});
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    // King heads to the empty target X and arrives at t=1000.
    clickSquare(engine, controller, 0, 1);
    clickSquare(engine, controller, 0, 2);

    // At t=500, launch the rook toward X while X is still empty (arrives t=1500).
    engine.advanceTime(static_cast<int>(500));
    clickSquare(engine, controller, 0, 3);
    clickSquare(engine, controller, 0, 2);
    CHECK(isPieceMovingAt(engine, 0, 3));

    // At t=1000 the king lands on X, then immediately jumps (airborne 1000..2000).
    engine.advanceTime(static_cast<int>(500));
    CHECK(hasPieceAt(engine.board(), 0, 2, PieceType::King, Color::White));
    jumpSquare(engine, controller, 0, 2);
    CHECK(movementStateAt(engine, 0, 2) == TestMovementState::Airborne);

    // At t=1500 the friendly rook arrives at X while the ally is airborne: the
    // move is cancelled and the rook remains on its origin cell.
    engine.advanceTime(static_cast<int>(500));
    CHECK(hasRookAt(engine.board(), 0, 3, Color::White));
    CHECK(movementStateAt(engine, 0, 3) == TestMovementState::Idle);
    CHECK(hasPieceAt(engine.board(), 0, 2, PieceType::King, Color::White));
    CHECK(movementStateAt(engine, 0, 2) == TestMovementState::Airborne);
}

TEST_CASE("jump: a moving piece cannot initiate a jump") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    clickSquare(engine, controller, 0, 0);
    clickSquare(engine, controller, 0, 2);
    CHECK(isPieceMovingAt(engine, 0, 0));

    // Jump request must be rejected: the piece stays Moving, not Airborne.
    jumpSquare(engine, controller, 0, 0);
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Moving);

    // The original move still completes normally.
    engine.advanceTime(static_cast<int>(moveDurationMs(0, 0, 0, 2)));
    CHECK(hasRookAt(engine.board(), 0, 2));
    CHECK(movementStateAt(engine, 0, 2) == TestMovementState::Idle);
}

TEST_CASE("jump: an empty (captured) cell cannot initiate a jump") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    // An empty square models a captured/absent piece: nothing should happen.
    jumpSquare(engine, controller, 0, 1);
    CHECK(engine.board().cell(pos(0, 1)).isEmpty());
    CHECK(movementStateAt(engine, 0, 1) == TestMovementState::Idle);
}

TEST_CASE("jump: an airborne piece cannot move") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    jumpSquare(engine, controller, 0, 0);
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Airborne);

    // Attempting to move the airborne piece must be rejected.
    clickSquare(engine, controller, 0, 0);
    clickSquare(engine, controller, 0, 2);
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Airborne);
    CHECK(engine.board().cell(pos(0, 2)).isEmpty());

    // It simply lands in place when the timer expires.
    engine.advanceTime(static_cast<int>(GameConfig::kJumpDurationMs));
    CHECK(hasRookAt(engine.board(), 0, 0));
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Idle);
}

TEST_CASE("jump: cannot start a second jump while already airborne") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    jumpSquare(engine, controller, 0, 0);  // airborne 0..1000

    // Halfway through, a second jump request must be ignored (does not extend it).
    engine.advanceTime(static_cast<int>(500));
    jumpSquare(engine, controller, 0, 0);
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Airborne);

    // If the second jump had registered (500..1500) the piece would still be
    // airborne at t=1000; instead the original jump lands exactly on time.
    engine.advanceTime(static_cast<int>(500));
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Idle);
    CHECK(hasRookAt(engine.board(), 0, 0));
}

TEST_CASE("jump: enemy arriving exactly at the boundary tick is jump-captured") {
    // Inclusive window: the piece is airborne for the full [start, finish]
    // duration, so an enemy arriving on the exact finish tick is still captured.
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece(PieceType::Rook, Color::Black),
                  Piece::empty(), Piece::empty()});
    engine::GameEngine engine;
    input::Controller controller;
    copyBoardIntoEngine(engine, board);

    jumpSquare(engine, controller, 0, 0);  // white airborne 0..1000
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Airborne);

    // Black rook (0,1) -> (0,0) is a 1-cell trip, arriving exactly at t=1000.
    clickSquare(engine, controller, 0, 1);
    clickSquare(engine, controller, 0, 0);
    CHECK(isPieceMovingAt(engine, 0, 1));

    engine.advanceTime(static_cast<int>(GameConfig::kJumpDurationMs));

    // The airborne white rook captured the arriving black rook, then landed.
    CHECK(hasRookAt(engine.board(), 0, 0, Color::White));
    CHECK_FALSE(hasRookAt(engine.board(), 0, 0, Color::Black));
    CHECK(engine.board().cell(pos(0, 1)).isEmpty());
    CHECK(movementStateAt(engine, 0, 0) == TestMovementState::Idle);
}

TEST_CASE("jump: command uses pixel coordinates like click") {
    // 'jump 61 164' -> col 0 row 1 (window pixel center of that cell)
    // and captures the black rook that arrives on the exact boundary tick.
    CHECK(runInput(" Board:\n. . .\nwK bR .\n. . .\nCommands:\njump 61 164\nclick 161 164\n"
                   "click 61 164\nwait 1000\nprint board") == ". . .\nwK . .\n. . .\n");
}

TEST_CASE("jump: command integration via the command processor") {
    // Black rook races 3 cells toward the white rook (arrives t=3000); the white
    // rook jumps at t=2500 and destroys the arriving enemy while airborne.
    CHECK(runInput(" Board:\nwR . . bR\nCommands:\nclick 361 64\nclick 61 64\nwait 2500\n"
                   "jump 61 64\nwait 500\nprint board\nwait 500\nprint board") ==
          "wR . . .\nwR . . .\n");
}
