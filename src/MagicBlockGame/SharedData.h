#pragma once

#include <stdint.h>
#include <stddef.h>

#include <vector>

#include "Color.h"
#include "Move.h"
#include "Board.h"

namespace PuzzleGame {

template <size_t BoardX, size_t BoardY,
          size_t TargetX, size_t TargetY>
struct SharedData
{
    Board<BoardX, BoardY> board;
    Board<TargetX, TargetY> target;

    int board_colors[Color::Maximum];
    int target_colors[Color::Maximum];

    std::vector<Move> empty_moves[BoardX * BoardY];
};

} // namespace PuzzleGame
