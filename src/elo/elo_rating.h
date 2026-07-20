#pragma once

namespace elo {

struct EloUpdate {
    int winner_new = 0;
    int loser_new = 0;
};

EloUpdate computeUpdate(int winner_rating, int loser_rating, double k = 32.0);

}  // namespace elo
