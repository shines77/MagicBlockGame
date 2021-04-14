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

    size_t has_solution;
    size_t depth_limit;
    std::vector<stage_type> stage_list[4];

    Step123() : has_solution(0), depth_limit(size_t(-1)) {}
    ~Step123() {}
};

template <size_t BoardX, size_t BoardY>
struct Step456
{
    size_t openning_type;
    size_t index;
    size_t depth_limit;
    int lock_inited[4];
    int locked[BoardX * BoardY];

    Step456() : openning_type(size_t(-1)), index(size_t(-1)), depth_limit(size_t(-1)) {
        for (size_t i = 0; i < 4; i++) {
            this->lock_inited[i] = 0;
        }

        for (size_t i = 0; i < BoardX * BoardY; i++) {
            this->locked[i] = 0;
        }
    }

    ~Step456() {}
};

template <size_t BoardX, size_t BoardY,
          size_t TargetX, size_t TargetY>
struct SharedData
{
    typedef typename Step123<BoardX, BoardY>::stage_type stage_type;

    Board<BoardX, BoardY> player;
    Board<TargetX, TargetY> target[4];

    size_t target_len;

    int player_colors[Color::Maximum];
    int target_colors[Color::Maximum];    

    std::vector<Move> empty_moves[BoardX * BoardY];

    Step123<BoardX, BoardY> s123;
    Step456<BoardX, BoardY> s456;

    SharedData() : target_len(0) {}
    ~SharedData() {}
};

} // namespace PuzzleGame
