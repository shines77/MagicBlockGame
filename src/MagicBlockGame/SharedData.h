#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstdint>
#include <cstddef>
#include <vector>

#include "Constant.h"
#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "Stage.h"

namespace MagicBlock {

template <std::size_t BoardX, std::size_t BoardY>
struct Step123
{
    typedef Stage<BoardX, BoardY> stage_type;

    std::size_t has_solution[MAX_ROTATE_TYPE];
    std::size_t depth_limit[MAX_ROTATE_TYPE];

    int min_depth[MAX_ROTATE_TYPE][MAX_PHASE1_TYPE];
    int max_depth[MAX_ROTATE_TYPE][MAX_PHASE1_TYPE];

    std::vector<stage_type> stage_list[MAX_ROTATE_TYPE][MAX_PHASE1_TYPE];

    Step123() {
        this->init(std::size_t(-1));
    }
    ~Step123() {}

    void init(std::size_t defaultSearchDepthLimit) {
        for (std::size_t rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
            this->has_solution[rotate_type] = 0;
            this->depth_limit[rotate_type] = defaultSearchDepthLimit;

            for (std::size_t phase1_type = 0; phase1_type < MAX_PHASE1_TYPE; phase1_type++) {
                this->min_depth[rotate_type][phase1_type] = -1;
                this->max_depth[rotate_type][phase1_type] = -1;
            }
        }
    }
};

template <std::size_t BoardX, std::size_t BoardY>
struct Step456
{
    std::size_t rotate_type;
    std::size_t phase1_type;
    std::size_t index;
    std::size_t depth_limit;
    int lock_inited[4];
    int locked[BoardX * BoardY];

    Step456() : rotate_type(std::size_t(-1)), phase1_type(std::size_t(-1)),
                index(std::size_t(-1)), depth_limit(std::size_t(-1)) {
        this->reset();
    }

    ~Step456() {}

    void reset() {
        //this->rotate_type = std::size_t(-1);
        //this->phase1_type = std::size_t(-1);
        this->depth_limit = std::size_t(-1);
        for (std::size_t i = 0; i < 4; i++) {
            this->lock_inited[i] = 0;
        }

        for (std::size_t i = 0; i < BoardX * BoardY; i++) {
            this->locked[i] = 0;
        }
    }
};

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t TargetX, std::size_t TargetY>
struct SharedData
{
    typedef typename Step123<BoardX, BoardY>::stage_type stage_type;

    Board<BoardX, BoardY> player_board;
    Board<TargetX, TargetY> target_board[4];

    std::size_t target_len;

    int player_colors[Color::Maximum];
    int target_colors[Color::Maximum];    

    std::vector<Move> empty_moves[BoardX * BoardY];

    Step123<BoardX, BoardY> s123;
    Step456<BoardX, BoardY> s456;

    SharedData() : target_len(0) {}
    ~SharedData() {}
};

} // namespace MagicBlock
