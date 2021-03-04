#pragma once

#include <stdint.h>
#include <stddef.h>

#include <vector>

#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "Stage.h"

namespace PuzzleGame {

template <size_t BoardX, size_t BoardY>
struct Step123
{
    typedef Stage<BoardX, BoardY> stage_type;

    int min_depth[4];
    int max_depth[4];

    size_t depth_limit;
    std::vector<stage_type> stage_list[4];
};

template <size_t BoardX, size_t BoardY>
struct Step456
{
    size_t openning_type;
    size_t index;
    int lock_inited;
    int locked[BoardX * BoardY];

    Step456() : openning_type(size_t(-1)), index(size_t(-1)), lock_inited(0) {}
    ~Step456() {}
};

template <size_t BoardX, size_t BoardY,
          size_t TargetX, size_t TargetY>
struct SharedData
{
    typedef typename Step123<BoardX, BoardY>::stage_type stage_type;

    Board<BoardX, BoardY> board;
    Board<TargetX, TargetY> target;

    int board_colors[Color::Maximum];
    int target_colors[Color::Maximum];

    std::vector<Move> empty_moves[BoardX * BoardY];

    Step123<BoardX, BoardY> s123;
    Step456<BoardX, BoardY> s456;

    SharedData() {}
    ~SharedData() {}
};

} // namespace PuzzleGame
