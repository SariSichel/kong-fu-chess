#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <iosfwd>
#include <string>

class Board;
class GameState;

namespace CommandProcessor {

void processLine(const std::string& line, std::ostream& out, Board& board, GameState& state);
void processCommands(std::istream& in, std::ostream& out, Board& board, GameState& state);
int run(std::istream& in, std::ostream& out);

}  // namespace CommandProcessor

#endif
