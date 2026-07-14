#include "renderer.h"

#include <opencv2/opencv.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "../constants.h"
#include "../model/piece.h"
#include "../model/position.h"
#include "img.hpp"

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

std::string pieceToSpriteCode(const model::Piece& piece) {
    const char typeChar = pieceTypeToChar(piece.type());
    const char colorChar = piece.color() == model::Color::White ? 'W' : 'B';
    return std::string({typeChar, colorChar});
}

std::string idleSpritePath(const std::string& pieceCode) {
    return std::string(kPiecesRoot) + "/" + pieceCode + "/states/idle/sprites/1.png";
}

// Img::draw_on alpha-blending requires BGR (3-channel) canvas when sprites are BGRA.
// Load board as BGR without modifying img.cpp — same pattern as CTD26 py/example.py.
Img loadBoardCanvas(const std::string& boardImagePath) {
    const cv::Mat board = cv::imread(boardImagePath, cv::IMREAD_UNCHANGED);
    if (board.empty()) {
        throw std::runtime_error("Cannot load board image: " + boardImagePath);
    }

    Img canvas;
    if (board.channels() == 4) {
        cv::Mat bgr;
        cv::cvtColor(board, bgr, cv::COLOR_BGRA2BGR);
        const auto tempPath =
            std::filesystem::temp_directory_path() / "kong_fu_chess_board.png";
        cv::imwrite(tempPath.string(), bgr);
        canvas.read(tempPath.string());
    } else {
        canvas.read(boardImagePath);
    }
    return canvas;
}

}  // namespace

void Renderer::render(const model::Board& board, const std::string& boardImagePath) {
    Img canvas = loadBoardCanvas(boardImagePath);

    drawPieces(canvas, board);

    canvas.show();
}

void Renderer::drawPieces(Img& canvas, const model::Board& board) {
    const int cellSize = GameConfig::kClickCellSize;
    const std::pair<int, int> spriteSize{cellSize, cellSize};

    for (size_t row = 0; row < board.rows(); ++row) {
        for (size_t col = 0; col < board.cols(); ++col) {
            const model::Piece& piece = board.cell(model::Position{static_cast<int>(row),
                                                                   static_cast<int>(col)});
            if (piece.isEmpty()) {
                continue;
            }

            Img sprite;
            sprite.read(idleSpritePath(pieceToSpriteCode(piece)), spriteSize, true,
                        cv::INTER_AREA);

            const int x = static_cast<int>(col) * cellSize;
            const int y = static_cast<int>(row) * cellSize;
            sprite.draw_on(canvas, x, y);
        }
    }
}

}  // namespace view
