#pragma once

#include <stdint.h>
#include <stddef.h>

#include <vector>

#include "Constant.h"
#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "Stage.h"

namespace MagicBlock {

template <size_t BoardX, size_t BoardY>
struct Step123
{
    typedef Stage<BoardX, BoardY> stage_type;

    size_t has_solution[MaxRotateType];
    size_t depth_limit[MaxRotateType];

    int min_depth[MaxRotateType][MaxPhrase1Type];
    int max_depth[MaxRotateType][MaxPhrase1Type];

    std::vector<stage_type> stage_list[MaxRotateType][MaxPhrase1Type];

    Step123() {
        this->init(size_t(-1));
    }
    ~Step123() {}

    void init(size_t defaultSearchDepthLimit) {
        for (size_t rotate_type = 0; rotate_type < MaxRotateType; rotate_type++) {
            this->has_solution[rotate_type] = 0;
            this->depth_limit[rotate_type] = defaultSearchDepthLimit;

            for (size_t phrase1_type = 0; phrase1_type < MaxPhrase1Type; phrase1_type++) {
                this->min_depth[rotate_type][phrase1_type] = -1;
                this->max_depth[rotate_type][phrase1_type] = -1;
            }
        }
    }
};

template <size_t BoardX, size_t BoardY>
struct Step456
{
    size_t phrase1_type;
    size_t index;
    size_t depth_limit;
    int lock_inited[4];
    int locked[BoardX * BoardY];

    Step456() : phrase1_type(size_t(-1)), index(size_t(-1)), depth_limit(size_t(-1)) {
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

    Board<BoardX, BoardY> player_board;
    Board<TargetX, TargetY> target_board[4];

    size_t target_len;

    int player_colors[Color::Maximum];
    int target_colors[Color::Maximum];    

    std::vector<Move> empty_moves[BoardX * BoardY];

    Step123<BoardX, BoardY> s123;
    Step456<BoardX, BoardY> s456;

    SharedData() : target_len(0) {}
    ~SharedData() {}
};

} // namespace MagicBlock
