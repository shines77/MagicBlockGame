#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

#include "MagicBlock/AI/Constant.h"
#include "MagicBlock/AI/Color.h"
#include "MagicBlock/AI/Move.h"
#include "MagicBlock/AI/Board.h"
#include "MagicBlock/AI/Stage.h"

namespace MagicBlock {
namespace AI {

template <std::size_t BoardX, std::size_t BoardY>
struct Phase1
{
    typedef Stage<BoardX, BoardY> stage_type;

    std::size_t has_solution[MAX_ROTATE_TYPE];
    std::size_t depth_limit[MAX_ROTATE_TYPE];

    int min_depth[MAX_ROTATE_TYPE][MAX_PHASE1_TYPE];
    int max_depth[MAX_ROTATE_TYPE][MAX_PHASE1_TYPE];

    std::vector<stage_type> stage_list[MAX_ROTATE_TYPE][MAX_PHASE1_TYPE];

    Phase1() {
        this->init(std::size_t(-1));
    }

    Phase1(const Phase1 & src) {
        this->internal_copy(src);
    }

    ~Phase1() {
    }

    Phase1 & operator = (const Phase1 & rhs) {
        this->copy(rhs);
        return *this;
    }

    void init(std::size_t depth_limit) {
        for (std::size_t rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
            this->has_solution[rotate_type] = 0;
            this->depth_limit[rotate_type] = depth_limit;

            for (std::size_t phase1_type = 0; phase1_type < MAX_PHASE1_TYPE; phase1_type++) {
                this->min_depth[rotate_type][phase1_type] = -1;
                this->max_depth[rotate_type][phase1_type] = -1;
            }
        }
    }

    void internal_copy(const Phase1 & other) {
        for (std::size_t rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
            this->has_solution[rotate_type] = other.has_solution[rotate_type];
            this->depth_limit[rotate_type] = other.depth_limit[rotate_type];

            for (std::size_t phase1_type = 0; phase1_type < MAX_PHASE1_TYPE; phase1_type++) {
                this->stage_list[rotate_type][phase1_type] = other.stage_list[rotate_type][phase1_type];
                this->min_depth[rotate_type][phase1_type] = other.min_depth[rotate_type][phase1_type];
                this->max_depth[rotate_type][phase1_type] = other.max_depth[rotate_type][phase1_type];
            }
        }
    }

    void copy(const Phase1 & other) {
        if (&other != this) {
            this->internal_copy(other);
        }
    }
};

template <std::size_t BoardX, std::size_t BoardY>
struct Phase2
{
    std::size_t rotate_type;
    std::size_t phase1_type;
    std::size_t index;
    std::size_t depth_limit;
    int lock_inited[4];
    int locked[BoardX * BoardY];

    Phase2() : rotate_type(std::size_t(-1)), phase1_type(std::size_t(-1)),
               index(std::size_t(-1)), depth_limit(std::size_t(-1)) {
        this->reset();
    }
    Phase2(const Phase2 & src) {
        this->internal_copy(src);
    }

    Phase2 & operator = (const Phase2 & rhs) {
        this->copy(rhs);
        return *this;
    }

    ~Phase2() {
        this->index = 0;
    }

    void internal_copy(const Phase2 & other) {
        this->rotate_type = other.rotate_type;
        this->phase1_type = other.phase1_type;
        this->index = other.index;
        this->depth_limit = other.depth_limit;

        for (std::size_t i = 0; i < 4; i++) {
            this->lock_inited[i] = other.lock_inited[i];
        }

        for (std::size_t i = 0; i < BoardX * BoardY; i++) {
            this->locked[i] = other.locked[i];
        }
    }

    void copy(const Phase2 & other) {
        if (&other != this) {
            this->internal_copy(other);
        }
    }

    void reset() {
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
    typedef typename Phase1<BoardX, BoardY>::stage_type stage_type;

    Board<BoardX, BoardY>   player_board;
    Board<TargetX, TargetY> target_board[4];

    std::size_t target_len;

    int player_colors[Color::Maximum];
    int target_colors[Color::Maximum];

    std::vector<Move> empty_moves[BoardX * BoardY];

    Phase1<BoardX, BoardY> phase1;
    Phase2<BoardX, BoardY> phase2;

    SharedData() : target_len(0) {
        for (std::size_t clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->player_colors[clr] = Color::Empty;
            this->target_colors[clr] = Color::Empty;
        }
    }
    SharedData(const SharedData & src) {
        this->internal_copy(src);
    }

    ~SharedData() {
    }

    SharedData & operator = (const SharedData & rhs) {
        this->copy(rhs);
        return *this;
    }

    void internal_copy(const SharedData & other) {
        this->player_board = other.player_board;
        for (std::size_t i = 0; i < 4; i++) {
            this->target_board[i] = other.target_board[i];
        }

        this->target_len = other.target_len;

        for (std::size_t clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->player_colors[clr] = other.player_colors[clr];
            this->target_colors[clr] = other.target_colors[clr];
        }

        for (std::size_t i = 0; i < BoardX * BoardY; i++) {
            this->empty_moves[i] = other.empty_moves[i];
        }

        this->phase1 = other.phase1;
        this->phase2 = other.phase2;
    }

    void copy(const SharedData & other) {
        if (&other != this) {
            this->internal_copy(other);
        }
    }
};

} // namespace AI
} // namespace MagicBlock
