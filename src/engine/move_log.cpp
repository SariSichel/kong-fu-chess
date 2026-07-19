#include "move_log.h"

#include "../io/algebraic.h"
#include <sstream>
#include <string>

namespace engine {

void MoveLog::clear() {
    entries_.clear();
    white_score_ = 0;
    black_score_ = 0;
}

int MoveLog::captureValue(model::PieceType type) {
    switch (type) {
        case model::PieceType::Pawn:
            return 1;
        case model::PieceType::Knight:
        case model::PieceType::Bishop:
            return 3;
        case model::PieceType::Rook:
            return 5;
        case model::PieceType::Queen:
            return 9;
        case model::PieceType::King:
        case model::PieceType::Empty:
        default:
            return 0;
    }
}

std::string MoveLog::formatElapsedMMSS(int timestampMs) {
    const int totalSeconds = timestampMs / 1000;
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;

    std::ostringstream out;
    out.width(2);
    out.fill('0');
    out << minutes << ':';
    out.width(2);
    out.fill('0');
    out << seconds;
    return out.str();
}

std::string MoveLog::positionToAlgebraic(const model::Position& pos) {
    return io::positionToAlgebraic(pos);
}

std::string MoveLog::pieceLabel(model::PieceType type, model::Color color) {
    const char colorChar = color == model::Color::White ? 'w' : 'b';
    char typeChar = '?';
    switch (type) {
        case model::PieceType::King:
            typeChar = 'K';
            break;
        case model::PieceType::Queen:
            typeChar = 'Q';
            break;
        case model::PieceType::Rook:
            typeChar = 'R';
            break;
        case model::PieceType::Bishop:
            typeChar = 'B';
            break;
        case model::PieceType::Knight:
            typeChar = 'N';
            break;
        case model::PieceType::Pawn:
            typeChar = 'P';
            break;
        case model::PieceType::Empty:
        default:
            typeChar = '?';
            break;
    }

    return std::string({colorChar, typeChar});
}

void MoveLog::addCaptureScore(model::Color scorer, model::PieceType victimType) {
    const int points = captureValue(victimType);
    if (points <= 0) {
        return;
    }

    if (scorer == model::Color::White) {
        white_score_ += points;
    } else {
        black_score_ += points;
    }
}

void MoveLog::recordCompletedMove(const realtime::CompletedMoveEvent& event) {
    std::ostringstream line;
    line << formatElapsedMMSS(event.timestamp_ms) << ' '
         << pieceLabel(event.piece_type, event.piece_color) << ' '
         << positionToAlgebraic(event.from) << "->" << positionToAlgebraic(event.to);

    if (event.captured_type != model::PieceType::Empty) {
        line << " x" << pieceLabel(event.captured_type, event.captured_color);
        addCaptureScore(event.piece_color, event.captured_type);
    }

    entries_.push_back({event.timestamp_ms, line.str()});
}

void MoveLog::recordJumpCapture(const realtime::JumpCaptureEvent& event) {
    std::ostringstream line;
    line << formatElapsedMMSS(event.timestamp_ms) << ' '
         << pieceLabel(event.jumper_type, event.jumper_color) << " jump x "
         << pieceLabel(event.victim_type, event.victim_color) << ' '
         << positionToAlgebraic(event.victim_from) << "->"
         << positionToAlgebraic(event.victim_to) << " @"
         << positionToAlgebraic(event.jump_square);

    addCaptureScore(event.jumper_color, event.victim_type);
    entries_.push_back({event.timestamp_ms, line.str()});
}

}  // namespace engine
