#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <sstream>
#include <string>

#include <cstdlib>

#include "board.h"
#include "board_serializer.h"
#include "constants.h"

// Test-only access to GameState internals (e.g. requestMove for enemy pieces).
#define private public
#include "game_state.h"
#undef private

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

void requestMoveDirect(GameState& state, Board& board, int fromR, int fromC, int toR,
                       int toC) {
    state.requestMove(fromR, fromC, toR, toC, board);
}

// TDD placeholder — wire to GameState premove storage when the engine supports it.
bool hasPremoveQueued(const GameState& state, int fromR, int fromC) {
    (void)state;
    (void)fromR;
    (void)fromC;
    return false;
}

// Stub until GameState exposes premove queue — wire to premoves_ when engine adds it.
bool hasQueuedPremove(const GameState& state, int fromR, int fromC, int toR, int toC) {
    (void)state;
    (void)fromR;
    (void)fromC;
    (void)toR;
    (void)toC;
    return false;
}

Board makeEnemyCollisionBoard() {
    Board board;
    board.addRow({Piece(PieceType::Rook, Color::White), Piece::empty(), Piece::empty(),
                  Piece::empty()});
    board.addRow({Piece::empty(), Piece::empty(), Piece::empty(), Piece::empty()});
    board.addRow({Piece(PieceType::Rook, Color::Black), Piece::empty(), Piece::empty(),
                  Piece::empty()});
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

// --- Advanced real-time interaction tests (TDD — expected to fail until engine supports them) ---

TEST_CASE("test_enemy_collision_black_started_first") {
    Board board = makeEnemyCollisionBoard();
    GameState state;
    state.reset();

    constexpr int kWhiteR = 0;
    constexpr int kWhiteC = 0;
    constexpr int kBlackR = 2;
    constexpr int kBlackC = 0;
    constexpr int kDestR = 0;
    constexpr int kDestC = 3;
    const long kTravelMs = moveDurationMs(kWhiteR, kWhiteC, kDestR, kDestC);

    requestMoveDirect(state, board, kBlackR, kBlackC, kDestR, kDestC);
    state.advanceTime(500, board);
    requestMoveDirect(state, board, kWhiteR, kWhiteC, kDestR, kDestC);

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
    constexpr int kBlackR = 2;
    constexpr int kBlackC = 0;
    constexpr int kDestR = 0;
    constexpr int kDestC = 3;
    const long kTravelMs = moveDurationMs(kWhiteR, kWhiteC, kDestR, kDestC);

    requestMoveDirect(state, board, kWhiteR, kWhiteC, kDestR, kDestC);
    state.advanceTime(500, board);
    requestMoveDirect(state, board, kBlackR, kBlackC, kDestR, kDestC);

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
    constexpr int kBlackR = 2;
    constexpr int kBlackC = 0;
    constexpr int kDestR = 0;
    constexpr int kDestC = 3;
    const long kTravelMs = moveDurationMs(kWhiteR, kWhiteC, kDestR, kDestC);

    requestMoveDirect(state, board, kWhiteR, kWhiteC, kDestR, kDestC);
    requestMoveDirect(state, board, kBlackR, kBlackC, kDestR, kDestC);

    state.advanceTime(kTravelMs, board);

    // Tie-breaker: lower source row wins (white rook at row 0 vs black rook at row 2).
    CHECK(hasRookAt(board, kDestR, kDestC, Color::White));
    CHECK_FALSE(hasRookAt(board, kDestR, kDestC, Color::Black));
    CHECK_FALSE(hasRookAt(board, kBlackR, kBlackC, Color::Black));
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

    CHECK(hasPremoveQueued(state, kSquareA_R, kSquareA_C));

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

    CHECK(hasPremoveQueued(state, kRookR, kRookC));

    state.advanceTime(500, board);
    requestMoveDirect(state, board, kKingR, kKingC, kSquareC_R, kSquareC_C);

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

    requestMoveDirect(state, board, kRookR, kRookC, kTargetR, kTargetC);
    requestMoveDirect(state, board, kKingR, kKingC, kTargetR, kTargetC);

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

    requestMoveDirect(state, board, kRookR, kRookC, kTargetR, kTargetC);
    state.advanceTime(1000, board);
    requestMoveDirect(state, board, kKingR, kKingC, kTargetR, kTargetC);

    CHECK(kRookTravelMs == kKingTravelMs + 1000);

    state.advanceTime(kKingTravelMs, board);

    CHECK(hasRookAt(board, kTargetR, kTargetC));
    CHECK(hasPieceAt(board, kKingR, kKingC, PieceType::King, Color::White));
    CHECK(movementStateAt(board, kKingR, kKingC) == PieceMovementState::Idle);
    CHECK_FALSE(hasPieceAt(board, kTargetR, kTargetC, PieceType::King, Color::White));
}
