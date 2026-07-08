#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <sstream>
#include <string>

#include <cstdlib>

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

void jumpSquare(GameState& state, Board& board, int row, int col) {
    const int x = col * GameConfig::kClickCellSize + GameConfig::kClickCellSize / 2;
    const int y = row * GameConfig::kClickCellSize + GameConfig::kClickCellSize / 2;
    state.handleJump(board, x, y);
}

bool hasRookAt(const Board& board, int row, int col, Color color = Color::White) {
    const Piece& piece = board.cell(row, col);
    return piece.type() == PieceType::Rook && piece.color() == color;
}

bool hasPieceAt(const Board& board, int row, int col, PieceType type, Color color) {
    const Piece& piece = board.cell(row, col);
    return piece.type() == type && piece.color() == color;
}

long moveDurationMs(int fromR, int fromC, int toR, int toC) {
    const int dr = std::abs(toR - fromR);
    const int dc = std::abs(toC - fromC);
    const int distance = std::max(dr, dc);
    return static_cast<long>(distance) * GameConfig::kMoveDurationMs;
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
                   "1000\nprint board") == "wQ . .\n. . .\n. . .\n");
    CHECK(runInput(" Board:\n. . .\n. . .\nwP . .\nCommands:\nclick 50 250\nclick 50 50\nwait "
                   "1000\nprint board") == ". . .\n. . .\nwP . .\n");
    CHECK(runInput(" Board:\nbP . .\nwP . .\n. . .\nCommands:\nclick 50 150\nclick 50 50\nwait "
                   "1000\nprint board") == "bP . .\nwP . .\n. . .\n");
    CHECK(runInput(" Board:\n. . .\nwP . .\n. . .\nCommands:\nclick 50 150\nclick 150 50\nwait "
                   "1000\nprint board") == ". . .\nwP . .\n. . .\n");
    CHECK(runInput(" Board:\n. bP .\nwP . .\n. . .\nCommands:\nclick 50 150\nclick 150 50\nwait "
                   "1000\nprint board") == ". wQ .\n. . .\n. . .\n");
}

// --- Pawn advanced rules: double move, path clearance, promotion ---

TEST_CASE("pawn: valid single step forward (white, no promotion)") {
    // 3-row board => white start row is rows()-1 == 2. A single step is always legal;
    // the destination (row 1) is not the last row, so no promotion happens.
    CHECK(runInput(" Board:\n. .\n. .\nwP .\nCommands:\nclick 50 250\nclick 50 150\nwait "
                   "1000\nprint board") == ". .\nwP .\n. .\n");
}

TEST_CASE("pawn: valid single step forward (black, no promotion)") {
    // Black moves down (increasing row). Pawn at row 0 steps to row 1 (not last row).
    CHECK(runInput(" Board:\nbP .\n. .\n. .\nCommands:\nclick 50 50\nclick 50 150\nwait "
                   "1000\nprint board") == ". .\nbP .\n. .\n");
}

TEST_CASE("pawn: valid double step from start row (white)") {
    // 5-row board => white start row is the bottom row (rows()-1 == 4). Pawn jumps
    // 4 -> 2 (not the last row). Double-step duration is 2 * kMoveDurationMs.
    CHECK(runInput(" Board:\n. .\n. .\n. .\n. .\nwP .\nCommands:\nclick 50 450\nclick 50 "
                   "250\nwait 2000\nprint board") == ". .\n. .\nwP .\n. .\n. .\n");
}

TEST_CASE("pawn: valid double step from start row (black)") {
    // 5-row board => black start row is the top row (0). Pawn jumps 0 -> 2 (not last row).
    CHECK(runInput(" Board:\nbP .\n. .\n. .\n. .\n. .\nCommands:\nclick 50 50\nclick 50 "
                   "250\nwait 2000\nprint board") == ". .\n. .\nbP .\n. .\n. .\n");
}

TEST_CASE("pawn: double step blocked by intermediate cell is rejected") {
    // White pawn on the start row (row 4). Intermediate cell (row 3) is occupied, so
    // the 4 -> 2 double step must be rejected and nothing on the board changes.
    CHECK(runInput(" Board:\n. .\n. .\n. .\nbP .\nwP .\nCommands:\nclick 50 450\nclick 50 "
                   "250\nwait 2000\nprint board") == ". .\n. .\n. .\nbP .\nwP .\n");
}

TEST_CASE("pawn: double step blocked by occupied target is rejected") {
    // White pawn on the start row (row 4). Target cell (row 2) is occupied, so the
    // 4 -> 2 double step must be rejected and nothing on the board changes.
    CHECK(runInput(" Board:\n. .\n. .\nbP .\n. .\nwP .\nCommands:\nclick 50 450\nclick 50 "
                   "250\nwait 2000\nprint board") == ". .\n. .\nbP .\n. .\nwP .\n");
}

TEST_CASE("pawn: double step from a non-start row is rejected") {
    // 5-row board => white start row is 4. Pawn sits on row 2 (not start row), so a
    // 2 -> 0 double step is illegal and the pawn stays put.
    CHECK(runInput(" Board:\n. .\n. .\nwP .\n. .\n. .\nCommands:\nclick 50 250\nclick 50 "
                   "50\nwait 2000\nprint board") == ". .\n. .\nwP .\n. .\n. .\n");
}

TEST_CASE("pawn: single-step promotion into final row (white)") {
    // White pawn one step from the top row (row 0) promotes to a Queen on arrival.
    CHECK(runInput(" Board:\n. .\nwP .\nCommands:\nclick 50 150\nclick 50 50\nwait "
                   "1000\nprint board") == "wQ .\n. .\n");
}

TEST_CASE("pawn: single-step promotion into final row (black)") {
    // Black pawn one step from the bottom row (last row) promotes to a Queen.
    CHECK(runInput(" Board:\nbP .\n. .\nCommands:\nclick 50 50\nclick 50 150\nwait "
                   "1000\nprint board") == ". .\nbQ .\n");
}

TEST_CASE("pawn: double-step promotion into final row (white)") {
    // 3-row board => white start row is rows()-1 == 2 and the last row is 0. The 2 -> 0
    // double step is legal and lands directly on the promotion row.
    CHECK(runInput(" Board:\n. .\n. .\nwP .\nCommands:\nclick 50 250\nclick 50 50\nwait "
                   "2000\nprint board") == "wQ .\n. .\n. .\n");
}

TEST_CASE("pawn: double-step promotion into final row (black)") {
    // 3-row board => black start row is 0 and the last row is rows()-1 == 2. The 0 -> 2
    // double step is legal and lands directly on the promotion row.
    CHECK(runInput(" Board:\nbP .\n. .\n. .\nCommands:\nclick 50 50\nclick 50 250\nwait "
                   "2000\nprint board") == ". .\n. .\nbQ .\n");
}

TEST_CASE("pawn: promotion happens via GameState arrival (state-level check)") {
    // Verify at the object level that validation runs before any board mutation and
    // that the pawn becomes a Queen only once it settles on the last row.
    Board board;
    board.addRow({Piece::empty(), Piece::empty()});
    board.addRow({Piece(PieceType::Pawn, Color::White), Piece::empty()});

    GameState state;
    state.reset();

    clickSquare(state, board, 1, 0);
    clickSquare(state, board, 0, 0);

    // Move is validated and started, but the piece is still a Pawn mid-flight.
    CHECK(isPieceMovingAt(board, 1, 0));
    CHECK(board.cell(1, 0).type() == PieceType::Pawn);

    state.advanceTime(moveDurationMs(1, 0, 0, 0), board);

    CHECK(hasPieceAt(board, 0, 0, PieceType::Queen, Color::White));
    CHECK_FALSE(hasPieceAt(board, 0, 0, PieceType::Pawn, Color::White));
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Idle);
}

TEST_CASE("pawn: invalid double step leaves the board completely unchanged") {
    // Strict validation ordering: an illegal move must alter nothing on the board.
    Board board;
    board.addRow({Piece::empty(), Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty()});
    board.addRow({Piece(PieceType::Pawn, Color::White), Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty()});

    GameState state;
    state.reset();

    // Pawn at row 2 is NOT on the start row (rows()-1 == 4), so a 2 -> 0 jump is invalid.
    clickSquare(state, board, 2, 0);
    clickSquare(state, board, 0, 0);

    CHECK_FALSE(isPieceMovingAt(board, 2, 0));
    CHECK(hasPieceAt(board, 2, 0, PieceType::Pawn, Color::White));

    state.advanceTime(moveDurationMs(2, 0, 0, 0), board);

    CHECK(hasPieceAt(board, 2, 0, PieceType::Pawn, Color::White));
    CHECK_FALSE(hasPieceAt(board, 0, 0, PieceType::Pawn, Color::White));
    CHECK_FALSE(hasPieceAt(board, 0, 0, PieceType::Queen, Color::White));
}

TEST_CASE("timed movement before and after arrival") {
    CHECK(runInput(" Board:\nwR . .\nCommands:\nclick 50 50\nclick 250 50\nwait 1000\nprint "
                   "board\nwait 1000\nprint board") == "wR . .\n. . wR\n");
    CHECK(runInput(" Board:\nwB . .\n. . .\n. . .\nCommands:\nclick 50 50\nclick 250 250\nwait 1000\n"
                   "print board\nwait 1000\nprint board") ==
          "wB . .\n. . .\n. . .\n. . .\n. . .\n. . wB\n");
}

TEST_CASE("enemy collision via click integration") {
    CHECK(runInput(" Board:\nwR . . bR\nCommands:\nclick 350 50\nclick 50 50\nclick 50 50\nclick "
                   "350 50\nwait 3000\nprint board") == "bR . . .\n");
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

// --- Advanced real-time interaction tests ---

TEST_CASE("test_enemy_collision_black_started_first") {
    Board board = makeEnemyCollisionBoard();
    GameState state;
    state.reset();

    constexpr int kWhiteR = 0;
    constexpr int kWhiteC = 0;
    constexpr int kBlackR = 3;
    constexpr int kBlackC = 3;
    constexpr int kDestR = 0;
    constexpr int kDestC = 3;
    const long kTravelMs = moveDurationMs(kWhiteR, kWhiteC, kDestR, kDestC);

    clickSquare(state, board, kBlackR, kBlackC);
    clickSquare(state, board, kDestR, kDestC);
    state.advanceTime(500, board);
    clickSquare(state, board, kWhiteR, kWhiteC);
    clickSquare(state, board, kDestR, kDestC);

    state.advanceTime(kTravelMs, board);
    state.advanceTime(500, board);

    CHECK(hasRookAt(board, kDestR, kDestC, Color::Black));
    CHECK_FALSE(hasRookAt(board, kWhiteR, kWhiteC, Color::White));
    CHECK_FALSE(hasRookAt(board, kDestR, kDestC, Color::White));
}

TEST_CASE("test_enemy_collision_white_started_first") {
    Board board = makeEnemyCollisionBoard();
    GameState state;
    state.reset();

    constexpr int kWhiteR = 0;
    constexpr int kWhiteC = 0;
    constexpr int kBlackR = 3;
    constexpr int kBlackC = 3;
    constexpr int kDestR = 0;
    constexpr int kDestC = 3;
    const long kTravelMs = moveDurationMs(kWhiteR, kWhiteC, kDestR, kDestC);

    clickSquare(state, board, kWhiteR, kWhiteC);
    clickSquare(state, board, kDestR, kDestC);
    state.advanceTime(500, board);
    clickSquare(state, board, kBlackR, kBlackC);
    clickSquare(state, board, kDestR, kDestC);

    state.advanceTime(kTravelMs, board);
    state.advanceTime(500, board);

    CHECK(hasRookAt(board, kDestR, kDestC, Color::White));
    CHECK_FALSE(hasRookAt(board, kBlackR, kBlackC, Color::Black));
    CHECK_FALSE(hasRookAt(board, kDestR, kDestC, Color::Black));
}

TEST_CASE("test_enemy_collision_absolute_tie") {
    Board board = makeEnemyCollisionBoard();
    GameState state;
    state.reset();

    constexpr int kWhiteR = 0;
    constexpr int kWhiteC = 0;
    constexpr int kBlackR = 3;
    constexpr int kBlackC = 3;
    constexpr int kDestR = 0;
    constexpr int kDestC = 3;
    const long kTravelMs = moveDurationMs(kWhiteR, kWhiteC, kDestR, kDestC);

    clickSquare(state, board, kBlackR, kBlackC);
    clickSquare(state, board, kDestR, kDestC);
    clickSquare(state, board, kWhiteR, kWhiteC);
    clickSquare(state, board, kDestR, kDestC);

    state.advanceTime(kTravelMs, board);

    // Tie-breaker: earlier command order wins (black requested first at the same tick).
    CHECK(hasRookAt(board, kDestR, kDestC, Color::Black));
    CHECK_FALSE(hasRookAt(board, kDestR, kDestC, Color::White));
    CHECK_FALSE(hasRookAt(board, kWhiteR, kWhiteC, Color::White));
}

TEST_CASE("test_premove_executes_on_arrival") {
    Board board = makeCommonRouteBoard();
    GameState state;
    state.reset();

    clickSquare(state, board, kSquareA_R, kSquareA_C);
    clickSquare(state, board, kSquareB_R, kSquareB_C);

    CHECK(isPieceMovingAt(board, kSquareA_R, kSquareA_C));

    clickSquare(state, board, kSquareA_R, kSquareA_C);
    clickSquare(state, board, kSquareC_R, kSquareC_C);

    CHECK(state.hasPremoveAt(kSquareA_R, kSquareA_C));

    state.advanceTime(kMoveAToBDurationMs, board);

    CHECK(hasRookAt(board, kSquareB_R, kSquareB_C));
    CHECK(isPieceMovingAt(board, kSquareB_R, kSquareB_C));
    CHECK(pieceDestinationRow(board, kSquareB_R, kSquareB_C) == kSquareC_R);
    CHECK(pieceDestinationCol(board, kSquareB_R, kSquareB_C) == kSquareC_C);

    state.advanceTime(kMoveBToCDurationMs, board);

    CHECK(hasRookAt(board, kSquareC_R, kSquareC_C));
    CHECK(movementStateAt(board, kSquareC_R, kSquareC_C) == PieceMovementState::Idle);
}

TEST_CASE("test_premove_cancelled_when_target_blocked") {
    Board board = makePremoveCancellationBoard();
    GameState state;
    state.reset();

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kSquareB_R = 0;
    constexpr int kSquareB_C = 2;
    constexpr int kSquareC_R = 0;
    constexpr int kSquareC_C = 3;
    constexpr int kKingR = 1;
    constexpr int kKingC = 3;
    const long kRookToBMs = moveDurationMs(kRookR, kRookC, kSquareB_R, kSquareB_C);
    const long kKingToCMs = moveDurationMs(kKingR, kKingC, kSquareC_R, kSquareC_C);

    clickSquare(state, board, kRookR, kRookC);
    clickSquare(state, board, kSquareB_R, kSquareB_C);

    clickSquare(state, board, kRookR, kRookC);
    clickSquare(state, board, kSquareC_R, kSquareC_C);

    CHECK(state.hasPremoveAt(kRookR, kRookC));

    state.advanceTime(500, board);
    clickSquare(state, board, kKingR, kKingC);
    clickSquare(state, board, kSquareC_R, kSquareC_C);

    state.advanceTime(kKingToCMs, board);
    CHECK(hasPieceAt(board, kSquareC_R, kSquareC_C, PieceType::King, Color::White));

    state.advanceTime(kRookToBMs - 500, board);

    CHECK(hasRookAt(board, kSquareB_R, kSquareB_C));
    CHECK(movementStateAt(board, kSquareB_R, kSquareB_C) == PieceMovementState::Idle);
    CHECK_FALSE(isPieceMovingAt(board, kSquareB_R, kSquareB_C));
    CHECK_FALSE(hasRookAt(board, kSquareC_R, kSquareC_C));
}

TEST_CASE("test_friendly_landing_blocked_at_arrival") {
    Board board = makeFriendlyLandingBoard();
    GameState state;
    state.reset();

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 1;
    constexpr int kTargetR = 0;
    constexpr int kTargetC = 2;
    const long kRookTravelMs = moveDurationMs(kRookR, kRookC, kTargetR, kTargetC);
    const long kKingTravelMs = moveDurationMs(kKingR, kKingC, kTargetR, kTargetC);

    clickSquare(state, board, kRookR, kRookC);
    clickSquare(state, board, kTargetR, kTargetC);
    clickSquare(state, board, kKingR, kKingC);
    clickSquare(state, board, kTargetR, kTargetC);

    state.advanceTime(kKingTravelMs, board);
    CHECK(hasPieceAt(board, kTargetR, kTargetC, PieceType::King, Color::White));

    state.advanceTime(kRookTravelMs - kKingTravelMs, board);

    CHECK(hasRookAt(board, kRookR, kRookC));
    CHECK(movementStateAt(board, kRookR, kRookC) == PieceMovementState::Idle);
    CHECK(hasPieceAt(board, kTargetR, kTargetC, PieceType::King, Color::White));
    CHECK_FALSE(hasRookAt(board, kTargetR, kTargetC));
}

TEST_CASE("test_arrival_processing_order_by_started_at") {
    Board board = makeArrivalOrderBoard();
    GameState state;
    state.reset();

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 1;
    constexpr int kKingC = 2;
    constexpr int kTargetR = 0;
    constexpr int kTargetC = 2;
    const long kRookTravelMs = moveDurationMs(kRookR, kRookC, kTargetR, kTargetC);
    const long kKingTravelMs = moveDurationMs(kKingR, kKingC, kTargetR, kTargetC);

    clickSquare(state, board, kRookR, kRookC);
    clickSquare(state, board, kTargetR, kTargetC);
    state.advanceTime(1000, board);
    clickSquare(state, board, kKingR, kKingC);
    clickSquare(state, board, kTargetR, kTargetC);

    CHECK(kRookTravelMs == kKingTravelMs + 1000);

    state.advanceTime(kKingTravelMs, board);

    CHECK(hasRookAt(board, kTargetR, kTargetC));
    CHECK(hasPieceAt(board, kKingR, kKingC, PieceType::King, Color::White));
    CHECK(movementStateAt(board, kKingR, kKingC) == PieceMovementState::Idle);
    CHECK_FALSE(hasPieceAt(board, kTargetR, kTargetC, PieceType::King, Color::White));
}

// --- Game-over behavior tests (TDD: requires GameState::isGameOver()) ---

TEST_CASE("game over: basic king capture ends the game") {
    Board board = makeBasicKingCaptureBoard();
    GameState state;
    state.reset();

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 3;
    const long kCaptureMs = moveDurationMs(kRookR, kRookC, kKingR, kKingC);

    CHECK_FALSE(state.isGameOver());

    clickSquare(state, board, kRookR, kRookC);
    clickSquare(state, board, kKingR, kKingC);

    CHECK_FALSE(state.isGameOver());
    CHECK(isPieceMovingAt(board, kRookR, kRookC));

    state.advanceTime(kCaptureMs, board);

    CHECK(state.isGameOver());
    CHECK(hasRookAt(board, kKingR, kKingC, Color::White));
    CHECK_FALSE(hasPieceAt(board, kKingR, kKingC, PieceType::King, Color::Black));
}

TEST_CASE("game over: moves are ignored after king capture") {
    Board board = makeIgnoreMovesAfterGameOverBoard();
    GameState state;
    state.reset();

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 3;
    constexpr int kKnightR = 1;
    constexpr int kKnightC = 1;
    constexpr int kKnightDestR = 1;
    constexpr int kKnightDestC = 2;
    const long kCaptureMs = moveDurationMs(kRookR, kRookC, kKingR, kKingC);

    clickSquare(state, board, kRookR, kRookC);
    clickSquare(state, board, kKingR, kKingC);
    state.advanceTime(kCaptureMs, board);

    CHECK(state.isGameOver());
    CHECK(hasRookAt(board, kKingR, kKingC, Color::White));
    CHECK(hasPieceAt(board, kKnightR, kKnightC, PieceType::Knight, Color::White));

    clickSquare(state, board, kKnightR, kKnightC);
    clickSquare(state, board, kKnightDestR, kKnightDestC);

    CHECK(hasPieceAt(board, kKnightR, kKnightC, PieceType::Knight, Color::White));
    CHECK_FALSE(isPieceMovingAt(board, kKnightR, kKnightC));
    CHECK_FALSE(hasPieceAt(board, kKnightDestR, kKnightDestC, PieceType::Knight, Color::White));

    state.advanceTime(moveDurationMs(kKnightR, kKnightC, kKnightDestR, kKnightDestC), board);

    CHECK(hasPieceAt(board, kKnightR, kKnightC, PieceType::Knight, Color::White));
    CHECK_FALSE(hasPieceAt(board, kKnightDestR, kKnightDestC, PieceType::Knight, Color::White));
    CHECK(state.isGameOver());
}

TEST_CASE("game over: friendly collision on own king does not end game") {
    Board board = makeFriendlyLandingBoard();
    GameState state;
    state.reset();

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 1;
    constexpr int kTargetR = 0;
    constexpr int kTargetC = 2;
    const long kRookTravelMs = moveDurationMs(kRookR, kRookC, kTargetR, kTargetC);
    const long kKingTravelMs = moveDurationMs(kKingR, kKingC, kTargetR, kTargetC);

    CHECK_FALSE(state.isGameOver());

    clickSquare(state, board, kRookR, kRookC);
    clickSquare(state, board, kTargetR, kTargetC);
    clickSquare(state, board, kKingR, kKingC);
    clickSquare(state, board, kTargetR, kTargetC);

    state.advanceTime(kKingTravelMs, board);
    CHECK(hasPieceAt(board, kTargetR, kTargetC, PieceType::King, Color::White));

    state.advanceTime(kRookTravelMs - kKingTravelMs, board);

    CHECK_FALSE(state.isGameOver());
    CHECK(hasRookAt(board, kRookR, kRookC));
    CHECK(hasPieceAt(board, kTargetR, kTargetC, PieceType::King, Color::White));
    CHECK_FALSE(hasRookAt(board, kTargetR, kTargetC));
}

TEST_CASE("game over: simultaneous arrival tie does not capture the king") {
    Board board = makeArrivalOrderBoard();
    GameState state;
    state.reset();

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 1;
    constexpr int kKingC = 2;
    constexpr int kTargetR = 0;
    constexpr int kTargetC = 2;
    const long kRookTravelMs = moveDurationMs(kRookR, kRookC, kTargetR, kTargetC);
    const long kKingTravelMs = moveDurationMs(kKingR, kKingC, kTargetR, kTargetC);

    CHECK_FALSE(state.isGameOver());

    // Rook starts first; king races to the same square and arrives on the same tick.
    clickSquare(state, board, kRookR, kRookC);
    clickSquare(state, board, kTargetR, kTargetC);
    state.advanceTime(1000, board);
    clickSquare(state, board, kKingR, kKingC);
    clickSquare(state, board, kTargetR, kTargetC);

    CHECK(kRookTravelMs == kKingTravelMs + 1000);

    state.advanceTime(kKingTravelMs, board);

    // Tie-breaker: rook started first, so it occupies the square. The king is not captured.
    CHECK_FALSE(state.isGameOver());
    CHECK(hasRookAt(board, kTargetR, kTargetC));
    CHECK(hasPieceAt(board, kKingR, kKingC, PieceType::King, Color::White));
    CHECK(movementStateAt(board, kKingR, kKingC) == PieceMovementState::Idle);
    CHECK_FALSE(hasPieceAt(board, kTargetR, kTargetC, PieceType::King, Color::White));
}

TEST_CASE("game over: king steps away before attacker arrives") {
    Board board = makeKingStepsAwayBoard();
    GameState state;
    state.reset();

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 2;
    constexpr int kKingDestR = 0;
    constexpr int kKingDestC = 3;
    const long kRookTravelMs = moveDurationMs(kRookR, kRookC, kKingR, kKingC);
    const long kKingTravelMs = moveDurationMs(kKingR, kKingC, kKingDestR, kKingDestC);

    CHECK_FALSE(state.isGameOver());

    clickSquare(state, board, kRookR, kRookC);
    clickSquare(state, board, kKingR, kKingC);
    clickSquare(state, board, kKingR, kKingC);
    clickSquare(state, board, kKingDestR, kKingDestC);

    state.advanceTime(kKingTravelMs, board);

    CHECK(hasPieceAt(board, kKingDestR, kKingDestC, PieceType::King, Color::Black));
    CHECK_FALSE(hasPieceAt(board, kKingR, kKingC, PieceType::King, Color::Black));
    CHECK(isPieceMovingAt(board, kRookR, kRookC));

    state.advanceTime(kRookTravelMs - kKingTravelMs, board);

    CHECK_FALSE(state.isGameOver());
    CHECK(hasRookAt(board, kKingR, kKingC, Color::White));
    CHECK(hasPieceAt(board, kKingDestR, kKingDestC, PieceType::King, Color::Black));
    CHECK_FALSE(hasPieceAt(board, kKingR, kKingC, PieceType::King, Color::Black));
}

TEST_CASE("game over: reset clears game-over flag") {
    Board board = makeBasicKingCaptureBoard();
    GameState state;
    state.reset();

    constexpr int kRookR = 0;
    constexpr int kRookC = 0;
    constexpr int kKingR = 0;
    constexpr int kKingC = 3;
    const long kCaptureMs = moveDurationMs(kRookR, kRookC, kKingR, kKingC);

    clickSquare(state, board, kRookR, kRookC);
    clickSquare(state, board, kKingR, kKingC);
    state.advanceTime(kCaptureMs, board);

    CHECK(state.isGameOver());

    state.reset();

    CHECK_FALSE(state.isGameOver());
}

// --- Jump ("Airborne") mechanic tests ---

TEST_CASE("jump: normal landing returns to idle on the same cell") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    GameState state;
    state.reset();

    jumpSquare(state, board, 0, 0);
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Airborne);

    // Still airborne one tick before the timer expires.
    state.advanceTime(GameConfig::kJumpDurationMs - 1, board);
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Airborne);

    state.advanceTime(1, board);
    CHECK(hasRookAt(board, 0, 0));
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Idle);
}

TEST_CASE("jump: airborne piece captures an arriving enemy and stays put") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece(PieceType::Rook, Color::Black)});
    GameState state;
    state.reset();

    // Black rook starts at t=0 heading for (0,0); a 3-cell trip arrives at t=3000.
    clickSquare(state, board, 0, 3);
    clickSquare(state, board, 0, 0);
    CHECK(isPieceMovingAt(board, 0, 3));

    // Jump at t=2500 so the white rook is airborne across the 3000ms arrival.
    state.advanceTime(2500, board);
    jumpSquare(state, board, 0, 0);
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Airborne);

    // At t=3000 the enemy arrives while the defender is still airborne.
    state.advanceTime(500, board);
    CHECK(hasRookAt(board, 0, 0, Color::White));
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Airborne);
    CHECK_FALSE(hasRookAt(board, 0, 3, Color::Black));
    CHECK(board.cell(0, 3).isEmpty());

    // The jump keeps running to its full duration, then lands in place.
    state.advanceTime(500, board);
    CHECK(hasRookAt(board, 0, 0, Color::White));
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Idle);
}

TEST_CASE("jump: friendly move in flight toward an airborne ally is cancelled") {
    // Layout: . wK . wR  -> king at (0,1), rook at (0,3), target square X = (0,2).
    Board board;
    board.addRow({Piece::empty(), Piece(PieceType::King, Color::White), Piece::empty(),
                  Piece(PieceType::Rook, Color::White)});
    GameState state;
    state.reset();

    // King heads to the empty target X and arrives at t=1000.
    clickSquare(state, board, 0, 1);
    clickSquare(state, board, 0, 2);

    // At t=500, launch the rook toward X while X is still empty (arrives t=1500).
    state.advanceTime(500, board);
    clickSquare(state, board, 0, 3);
    clickSquare(state, board, 0, 2);
    CHECK(isPieceMovingAt(board, 0, 3));

    // At t=1000 the king lands on X, then immediately jumps (airborne 1000..2000).
    state.advanceTime(500, board);
    CHECK(hasPieceAt(board, 0, 2, PieceType::King, Color::White));
    jumpSquare(state, board, 0, 2);
    CHECK(movementStateAt(board, 0, 2) == PieceMovementState::Airborne);

    // At t=1500 the friendly rook arrives at X while the ally is airborne: the
    // move is cancelled and the rook remains on its origin cell.
    state.advanceTime(500, board);
    CHECK(hasRookAt(board, 0, 3, Color::White));
    CHECK(movementStateAt(board, 0, 3) == PieceMovementState::Idle);
    CHECK(hasPieceAt(board, 0, 2, PieceType::King, Color::White));
    CHECK(movementStateAt(board, 0, 2) == PieceMovementState::Airborne);
}

TEST_CASE("jump: a moving piece cannot initiate a jump") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    GameState state;
    state.reset();

    clickSquare(state, board, 0, 0);
    clickSquare(state, board, 0, 2);
    CHECK(isPieceMovingAt(board, 0, 0));

    // Jump request must be rejected: the piece stays Moving, not Airborne.
    jumpSquare(state, board, 0, 0);
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Moving);

    // The original move still completes normally.
    state.advanceTime(moveDurationMs(0, 0, 0, 2), board);
    CHECK(hasRookAt(board, 0, 2));
    CHECK(movementStateAt(board, 0, 2) == PieceMovementState::Idle);
}

TEST_CASE("jump: an empty (captured) cell cannot initiate a jump") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    GameState state;
    state.reset();

    // An empty square models a captured/absent piece: nothing should happen.
    jumpSquare(state, board, 0, 1);
    CHECK(board.cell(0, 1).isEmpty());
    CHECK(movementStateAt(board, 0, 1) == PieceMovementState::Idle);
}

TEST_CASE("jump: an airborne piece cannot move") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    GameState state;
    state.reset();

    jumpSquare(state, board, 0, 0);
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Airborne);

    // Attempting to move the airborne piece must be rejected.
    clickSquare(state, board, 0, 0);
    clickSquare(state, board, 0, 2);
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Airborne);
    CHECK(board.cell(0, 2).isEmpty());

    // It simply lands in place when the timer expires.
    state.advanceTime(GameConfig::kJumpDurationMs, board);
    CHECK(hasRookAt(board, 0, 0));
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Idle);
}

TEST_CASE("jump: cannot start a second jump while already airborne") {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    GameState state;
    state.reset();

    jumpSquare(state, board, 0, 0);  // airborne 0..1000

    // Halfway through, a second jump request must be ignored (does not extend it).
    state.advanceTime(500, board);
    jumpSquare(state, board, 0, 0);
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Airborne);

    // If the second jump had registered (500..1500) the piece would still be
    // airborne at t=1000; instead the original jump lands exactly on time.
    state.advanceTime(500, board);
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Idle);
    CHECK(hasRookAt(board, 0, 0));
}

TEST_CASE("jump: enemy arriving exactly at the boundary tick is jump-captured") {
    // Inclusive window: the piece is airborne for the full [start, finish]
    // duration, so an enemy arriving on the exact finish tick is still captured.
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece(PieceType::Rook, Color::Black),
                  Piece::empty(), Piece::empty()});
    GameState state;
    state.reset();

    jumpSquare(state, board, 0, 0);  // white airborne 0..1000
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Airborne);

    // Black rook (0,1) -> (0,0) is a 1-cell trip, arriving exactly at t=1000.
    clickSquare(state, board, 0, 1);
    clickSquare(state, board, 0, 0);
    CHECK(isPieceMovingAt(board, 0, 1));

    state.advanceTime(GameConfig::kJumpDurationMs, board);

    // The airborne white rook captured the arriving black rook, then landed.
    CHECK(hasRookAt(board, 0, 0, Color::White));
    CHECK_FALSE(hasRookAt(board, 0, 0, Color::Black));
    CHECK(board.cell(0, 1).isEmpty());
    CHECK(movementStateAt(board, 0, 0) == PieceMovementState::Idle);
}

TEST_CASE("jump: command uses pixel coordinates like click") {
    // 'jump 50 150' -> x=50 (col 0), y=150 (row 1): the white king jumps in place
    // and captures the black rook that arrives on the exact boundary tick.
    CHECK(runInput(" Board:\n. . .\nwK bR .\n. . .\nCommands:\njump 50 150\nclick 150 150\n"
                   "click 50 150\nwait 1000\nprint board") == ". . .\nwK . .\n. . .\n");
}

TEST_CASE("jump: command integration via the command processor") {
    // Black rook races 3 cells toward the white rook (arrives t=3000); the white
    // rook jumps at t=2500 and destroys the arriving enemy while airborne.
    CHECK(runInput(" Board:\nwR . . bR\nCommands:\nclick 350 50\nclick 50 50\nwait 2500\n"
                   "jump 50 50\nwait 500\nprint board\nwait 500\nprint board") ==
          "wR . . .\nwR . . .\n");
}
