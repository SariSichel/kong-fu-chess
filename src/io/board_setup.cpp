#include "board_setup.h"

#include "../model/piece.h"

namespace io {

using model::Board;
using model::Color;
using model::Piece;
using model::PieceType;

void setupStandardBoard(Board& board) {
    board.clear();

    board.addRow({Piece(PieceType::Rook, Color::Black), Piece(PieceType::Knight, Color::Black),
                  Piece(PieceType::Rook, Color::Black), Piece(PieceType::King, Color::Black),
                  Piece(PieceType::Queen, Color::Black), Piece(PieceType::Rook, Color::Black),
                  Piece(PieceType::Knight, Color::Black), Piece(PieceType::Rook, Color::Black)});

    board.addRow({Piece(PieceType::Pawn, Color::Black), Piece(PieceType::Pawn, Color::Black),
                  Piece(PieceType::Pawn, Color::Black), Piece(PieceType::Pawn, Color::Black),
                  Piece(PieceType::Pawn, Color::Black), Piece(PieceType::Pawn, Color::Black),
                  Piece(PieceType::Pawn, Color::Black), Piece(PieceType::Pawn, Color::Black)});

    for (int i = 0; i < 4; ++i) {
        board.addRow({Piece::empty(), Piece::empty(), Piece::empty(), Piece::empty(),
                      Piece::empty(), Piece::empty(), Piece::empty(), Piece::empty()});
    }

    board.addRow({Piece(PieceType::Pawn, Color::White), Piece(PieceType::Pawn, Color::White),
                  Piece(PieceType::Pawn, Color::White), Piece(PieceType::Pawn, Color::White),
                  Piece(PieceType::Pawn, Color::White), Piece(PieceType::Pawn, Color::White),
                  Piece(PieceType::Pawn, Color::White), Piece(PieceType::Pawn, Color::White)});

    board.addRow({Piece(PieceType::Rook, Color::White), Piece(PieceType::Knight, Color::White),
                  Piece(PieceType::Rook, Color::White), Piece(PieceType::King, Color::White),
                  Piece(PieceType::Queen, Color::White), Piece(PieceType::Rook, Color::White),
                  Piece(PieceType::Knight, Color::White), Piece(PieceType::Rook, Color::White)});
}

}  // namespace io
