#include "elo_rating.h"

#include <cmath>

namespace elo {

EloUpdate computeUpdate(int winner_rating, int loser_rating, double k) {
    const double expected =
        1.0 / (1.0 + std::pow(10.0, (loser_rating - winner_rating) / 400.0));

    const int winner_new =
        winner_rating + static_cast<int>(std::round(k * (1.0 - expected)));
    const int loser_new =
        loser_rating + static_cast<int>(std::round(k * (0.0 - (1.0 - expected))));

    return EloUpdate{winner_new, loser_new};
}

}  // namespace elo
