#pragma once

#include <stdint.h>
#include <stddef.h>

#include <vector>

#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "Stage.h"

namespace PuzzleGame {

template <size_t BoardX, size_t BoardY,
          size_t TargetX, size_t TargetY>
struct SharedData
{
    typedef Stage<BoardX, BoardY> stage_type;

    Board<BoardX, BoardY> board;
    Board<TargetX, TargetY> target;

    int board_colors[Color::Maximum];
    int target_colors[Color::Maximum];

    std::vector<Move> empty_moves[BoardX * BoardY];

    int s123_min_depth[4];
    int s123_max_depth[4];

    size_t s123_depth_limit;
    std::vector<stage_type> s123_stages[4];
};

} // namespace PuzzleGame
