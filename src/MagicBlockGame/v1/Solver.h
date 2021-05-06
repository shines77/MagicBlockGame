#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <queue>
#include <set>
#include <exception>
#include <stdexcept>
#include <algorithm>    // For std::swap(), until C++11. std::min()
#include <utility>      // For std::swap(), since C++11

#include "internal/BaseSolver.h"

#include "Constant.h"
#include "Color.h"
#include "Move.h"
#include "Value128.h"
#include "Board.h"
#include "Stage.h"
#include "SharedData.h"
#include "SparseBitset.h"
#include "Utils.h"

namespace MagicBlock {
namespace v1 {

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t TargetX, std::size_t TargetY,
          bool AllowRotate, std::size_t N_SolverType,
          typename Phase2CallBack>
class Solver : public internal::BaseSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack>
{
public:
    typedef internal::BaseSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack> base_type;
    typedef Solver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack>               this_type;

    typedef typename base_type::size_type           size_type;
    typedef typename base_type::ssize_type          ssize_type;

    typedef typename base_type::shared_data_type    shared_data_type;
    typedef typename base_type::stage_type          stage_type;
    typedef typename base_type::phase2_callback     phase2_callback;

    static const size_type kSingelColorNums = (BoardX * BoardY - 1) / (Color::Last - 1);

    static const ptrdiff_t kStartX = (BoardX - TargetX) / 2;
    static const ptrdiff_t kStartY = (BoardY - TargetY) / 2;

#ifdef NDEBUG
    static const size_type kMinSearchDepth = 15;
    static const size_type kMaxSearchDepth = 27;

    static const size_type kDefaultSearchDepthLimit = 30;

    static const size_type kSlideDepth = 6;
    static const size_type kMaxSlideDepth = 10;
#else
    static const size_type kMinSearchDepth = 15;
    static const size_type kMaxSearchDepth = 21;

    static const size_type kDefaultSearchDepthLimit = 22;

    static const size_type kSlideDepth = 1;
    static const size_type kMaxSlideDepth = 2;
#endif

private:
    void init() {
        assert(this->data_ != nullptr);
        if (this->is_phase1()) {
            this->player_board_ = this->data_->player_board;
            for (size_type i = 0; i < MAX_ROTATE_TYPE; i++) {
                this->target_board_[i] = this->data_->target_board[i];
            }
            if (AllowRotate)
                this->target_len_ = this->data_->target_len;
            else
                this->target_len_ = 1;

            this->count_target_color_nums(this->target_board_[0]);

            this->data_->phase1.init(kDefaultSearchDepthLimit);

            // Reset lock_inited[]
            for (size_type phase1_type = 0; phase1_type < MAX_PHASE1_TYPE; phase1_type++) {
                this->data_->phase2.lock_inited[phase1_type] = 0;
            }
        }
        else if (this->is_phase2()) {
            //
            // We need to use setPlayerBoard(...) to setting player board in phase 2.
            //
            //this->player_board_ = this->data_->player_board;
        }
    }

    void init_target_board_locked(size_t rotate_type) {
        assert(this->data_ != nullptr);
        if (this->is_phase2()) {
            this->data_->phase2.reset();

            assert(rotate_type >= 0 && rotate_type < MAX_ROTATE_TYPE);
            this->target_board_[0] = this->data_->target_board[rotate_type];
            this->target_len_ = 1;

            size_type phase1_type = this->data_->phase2.phase1_type;
            if (this->data_->phase2.lock_inited[phase1_type] == 0) {
                this->data_->phase2.lock_inited[phase1_type] = 1;
                switch (phase1_type) {
                    case 0:
                        // Top partial
                        this->count_partial_target_color_nums(this->target_board_[0], 0, TargetX, TargetY - 1, TargetY);
                        this->locked_partial_board(this->data_->phase2.locked, 0, BoardX, 0, kStartY + 1);
                        break;
                    case 1:
                        // Left partial
                        this->count_partial_target_color_nums(this->target_board_[0], TargetX - 1, TargetX, 0, TargetY);
                        this->locked_partial_board(this->data_->phase2.locked, 0, kStartX + 1, 0, BoardY);
                        break;
                    case 2:
                        // Right partial
                        this->count_partial_target_color_nums(this->target_board_[0], 0, 1, 0, TargetY);
                        this->locked_partial_board(this->data_->phase2.locked, kStartX + TargetX - 1, BoardX, 0, BoardY);
                        break;
                    case 3:
                        // Bottom partial
                        this->count_partial_target_color_nums(this->target_board_[0], 0, TargetX, 0, 1);
                        this->locked_partial_board(this->data_->phase2.locked, 0, BoardX, kStartY + TargetY - 1, BoardY);
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
        }
    }

public:
    Solver(shared_data_type * data) : base_type(data) {
        this->init();
    }

    ~Solver() {
        this->destory();
    }

    void destory() {
        // TODO:
    }

    void setRotateType(size_type rotate_type) {
        assert(rotate_type >= 0 && rotate_type < MAX_ROTATE_TYPE);
        this->rotate_type_ = rotate_type;
        this->init_target_board_locked(rotate_type);
    }

    bool record_phase1_min_info(size_type depth, size_type rotate_type, size_type satisfy_mask, const stage_type & stage) {
        size_type reached_mask = 0;
        size_type mask = 1;
        size_type phase1_type = 0;
        while (satisfy_mask != 0) {
            if ((satisfy_mask & mask) == mask) {
                // record min-move phrase1 stage
                this->data_->phase1.stage_list[rotate_type][phase1_type].push_back(stage);

                if (this->data_->phase1.min_depth[rotate_type][phase1_type] != -1) {
                    assert(this->data_->phase1.max_depth[rotate_type][phase1_type] != -1);
                    if ((int)depth >= this->data_->phase1.max_depth[rotate_type][phase1_type]) {
                        reached_mask |= mask;
                    }
                }
                else {
                    if (this->data_->phase1.has_solution[rotate_type] == 0) {
                        this->data_->phase1.has_solution[rotate_type] = 1;
                        // Update the depth limit
                        this->data_->phase1.depth_limit[rotate_type] = std::min(
                            std::max(depth + kMaxSlideDepth, kMinSearchDepth), kMaxSearchDepth);
                    }
                    this->data_->phase1.min_depth[rotate_type][phase1_type] = (int)depth;
                    this->data_->phase1.max_depth[rotate_type][phase1_type] = (int)(depth + kSlideDepth);
                }
            }
            phase1_type++;
            if (phase1_type >= MAX_PHASE1_TYPE)
                break;
            satisfy_mask &= ~mask;
            mask <<= 1;
        }

        return ((reached_mask & 0x0F) == 0x0F);
    }

    bool solve_full(size_type & rotate_type) {
        size_u result = this->is_satisfy_full(this->player_board_, this->target_board_, this->target_len_);
        if (result.low != 0) {
            rotate_type = result.high;
            return true;
        }

        bool solvable = false;
        size_type depth = 0;

        Position empty;
        bool found_empty = this->find_empty(this->player_board_, empty);
        if (found_empty) {
            std::set<Value128> visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = uint8_t(-1);
            start.board = this->player_board_;
            visited.insert(start.board.value128());

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size()) {    
                for (size_type i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    uint8_t empty_pos = stage.empty;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_type total_moves = empty_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        stage_type next_stage(stage.board);
                        uint8_t move_pos = empty_moves[n].pos;
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        Value128 board_value = next_stage.board.value128();
                        if (visited.count(board_value) > 0)
                            continue;

                        visited.insert(board_value);

                        next_stage.empty = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        next_stage.move_path.push_back(move_pos);

                        next_stages.push_back(next_stage);

                        size_u result = this->is_satisfy_full(next_stage.board, this->target_board_, this->target_len_);
                        if (result.low != 0) {
                            this->move_path_ = next_stage.move_path;
                            assert((depth + 1) == next_stage.move_path.size());
                            rotate_type = result.high;
                            solvable = true;
                            exit = true;
                            break;
                        }  
                    }

                    if (exit) {
                        break;
                    }
                }

                depth++;
                printf("depth = %u\n", (uint32_t)depth);
                printf("cur.size() = %u, next.size() = %u\n",
                       (uint32_t)(cur_stages.size()),
                       (uint32_t)(next_stages.size()));
                printf("visited.size() = %u\n\n", (uint32_t)(visited.size()));

                std::swap(cur_stages, next_stages);
                next_stages.clear();

                if (exit) {
                    break;
                }
            }

            if (solvable) {
                this->map_used_ = visited.size();
            }
        }

        return solvable;
    }

    bool solve(size_type & out_rotate_type) {
        size_u satisfy_result = this->is_satisfy(this->player_board_, this->target_board_, this->target_len_);
        if (satisfy_result.low != 0) {
            out_rotate_type = satisfy_result.high;
            return true;
        }

        bool solvable = false;
        size_type depth = 0;

        Position empty;
        bool found_empty = this->find_empty(this->player_board_, empty);
        if (found_empty) {
            std::set<Value128> visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = uint8_t(-1);
            start.rotate_type = 0;
            start.board = this->player_board_;
            visited.insert(start.board.value128());

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size()) {
                for (size_type i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    uint8_t empty_pos = stage.empty;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_type total_moves = empty_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = empty_moves[n].pos;
                        if (this->is_phase2()) {
                            if (this->data_->phase2.locked[move_pos] != 0)
                                continue;
                        }

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        Value128 board_value = next_stage.board.value128();
                        if (visited.count(board_value) > 0) {
                            continue;
                        }

                        visited.insert(board_value);

                        next_stage.empty = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        next_stage.move_path.push_back(move_pos);

                        next_stages.push_back(next_stage);

                        size_u satisfy_result = this->is_satisfy(next_stage.board, this->target_board_, this->target_len_);
                        size_type satisfy_mask = satisfy_result.low;
                        if (satisfy_mask != 0) {
                            solvable = true;
                            if (this->is_phase1()) {
                                size_type rotate_type = satisfy_result.high;
                                next_stage.rotate_type = (uint8_t)rotate_type;
                                bool all_reached = record_phase1_min_info(depth, rotate_type, satisfy_mask, next_stage);
                                if (all_reached) {
                                    exit = true;
                                }
                            }
                            else {
                                this->move_path_ = next_stage.move_path;
                                assert((depth + 1) == next_stage.move_path.size());
                                exit = true;
                                break;
                            }
                        }
                    }
                    if (!(this->is_phase1())) {
                        if (exit) {
                            break;
                        }
                    }
                }

                depth++;
                if (this->is_phase1()) {
                    printf("depth = %u\n", (uint32_t)depth);
                    printf("cur.size() = %u, next.size() = %u\n",
                           (uint32_t)(cur_stages.size()), (uint32_t)(next_stages.size()));
                    printf("visited.size() = %u\n\n", (uint32_t)(visited.size()));
                }
                else {
                    //printf(">> %u\n", (uint32_t)depth);
                }

                std::swap(cur_stages, next_stages);
                next_stages.clear();

                if (this->is_phase1()) {
                    size_type rotate_done = 0;
                    for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                        if (this->data_->phase1.depth_limit[rotate_type] != size_t(-1) &&
                            depth > this->data_->phase1.depth_limit[rotate_type]) {
                            rotate_done++;
                        }
                    }
                    if (AllowRotate) {
                        if (rotate_done >= MAX_ROTATE_TYPE)
                            exit = true;
                    }
                    else {
                        if (rotate_done >= 1)
                            exit = true;
                    }
                }
                else if (this->is_phase2()) {
                    if (depth >= this->data_->phase2.depth_limit) {
                        exit = true;
                    }
                }

                if (exit) {
                    break;
                }
            }

            this->map_used_ = visited.size();

            if (this->is_phase1()) {
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                for (size_type rotate_type = 0; rotate_type < 4; rotate_type++) {
                    for (size_type phase1_type = 0; phase1_type < 4; phase1_type++) {
                        printf("rotate_type = %u, phase1_type = %u, min_depth = %d, max_depth = %d, stage.size() = %u\n",
                                (uint32_t)rotate_type,
                                (uint32_t)phase1_type,
                                this->data_->phase1.min_depth[rotate_type][phase1_type],
                                this->data_->phase1.max_depth[rotate_type][phase1_type],
                                (uint32_t)this->data_->phase1.stage_list[rotate_type][phase1_type].size());
                    }
                    printf("\n");
                }
                out_rotate_type = 0;
            }
            else if (this->is_phase2()) {
                if (solvable) {
                    //printf("\n");
                    printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                    printf("rotate_type = %u\n", (uint32_t)this->data_->phase2.rotate_type);
                    printf("phase1_type = %u\n", (uint32_t)this->data_->phase2.phase1_type);
                    printf("depth_limit = %u\n", (uint32_t)this->data_->phase2.depth_limit);
                    printf("index = %u\n", (uint32_t)(this->data_->phase2.index + 1));
                    printf("next.size() = %u\n", (uint32_t)cur_stages.size());
                    if (solvable) {
                        printf("move_path.size() = %u\n", (uint32_t)this->move_path_.size());
                    }
                    printf("\n");
                }
            }
        }

        return solvable;
    }

    bool queue_solve(size_type & out_rotate_type) {
        size_u satisfy_result = this->is_satisfy(this->player_board_, this->target_board_, this->target_len_);
        if (satisfy_result.low != 0) {
            out_rotate_type = satisfy_result.high;
            return true;
        }

        bool solvable = false;
        size_type depth = 0;

        Position empty;
        bool found_empty = this->find_empty(this->player_board_, empty);
        if (found_empty) {
            std::set<Value128> visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = uint8_t(-1);
            start.rotate_type = 0;
            start.board = this->player_board_;
            visited.insert(start.board.value128());

            std::queue<stage_type> cur_stages;
            std::queue<stage_type> next_stages;

            cur_stages.push(start);

            bool exit = false;
            while (!cur_stages.empty()) {
                do {
                    const stage_type & stage = cur_stages.front();

                    uint8_t empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_type total_moves = empty_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = empty_moves[n].pos;
                        if (this->is_phase2()) {
                            if (this->data_->phase2.locked[move_pos] != 0)
                                continue;
                        }

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        Value128 board_value = next_stage.board.value128();
                        if (visited.count(board_value) > 0) {
                            continue;
                        }

                        visited.insert(board_value);

                        next_stage.empty = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        next_stage.move_path.push_back(move_pos);

                        next_stages.push(next_stage);

                        size_u satisfy_result = this->is_satisfy(next_stage.board, this->target_board_, this->target_len_);
                        size_type satisfy_mask = satisfy_result.low;
                        if (satisfy_mask != 0) {
                            solvable = true;
                            if (this->is_phase1()) {
                                size_type rotate_type = satisfy_result.high;
                                next_stage.rotate_type = (uint8_t)rotate_type;
                                bool all_reached = record_phase1_min_info(depth, rotate_type, satisfy_mask, next_stage);
                                if (all_reached) {
                                    exit = true;
                                }
                            }
                            else {
                                this->move_path_ = next_stage.move_path;
                                assert((depth + 1) == next_stage.move_path.size());
                                exit = true;
                                break;
                            }
                        }
                    }

                    cur_stages.pop();

                    if (!(this->is_phase1())) {
                        if (exit) {
                            break;
                        }
                    }
                } while (!cur_stages.empty());

                depth++;
                if (this->is_phase1()) {
                    printf("depth = %u\n", (uint32_t)depth);
                    printf("cur.size() = %u, next.size() = %u\n",
                           (uint32_t)(cur_stages.size()), (uint32_t)(next_stages.size()));
                    printf("visited.size() = %u\n\n", (uint32_t)(visited.size()));
                }
                else {
                    //printf(">> %u\n", (uint32_t)depth);
                }

                std::swap(cur_stages, next_stages);

                if (this->is_phase1()) {
                    size_type rotate_done = 0;
                    for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                        if (this->data_->phase1.depth_limit[rotate_type] != size_t(-1) &&
                            depth > this->data_->phase1.depth_limit[rotate_type]) {
                            rotate_done++;
                        }
                    }
                    if (AllowRotate) {
                        if (rotate_done >= MAX_ROTATE_TYPE)
                            exit = true;
                    }
                    else {
                        if (rotate_done >= 1)
                            exit = true;
                    }
                }
                else if (this->is_phase2()) {
                    if (depth >= this->data_->phase2.depth_limit) {
                        exit = true;
                    }
                }

                if (exit) {
                    break;
                }
            }

            this->map_used_ = visited.size();

            if (this->is_phase1()) {
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                for (size_type rotate_type = 0; rotate_type < 4; rotate_type++) {
                    for (size_type phase1_type = 0; phase1_type < 4; phase1_type++) {
                        printf("rotate_type = %u, phase1_type = %u, min_depth = %d, max_depth = %d, stage.size() = %u\n",
                                (uint32_t)rotate_type,
                                (uint32_t)phase1_type,
                                this->data_->phase1.min_depth[rotate_type][phase1_type],
                                this->data_->phase1.max_depth[rotate_type][phase1_type],
                                (uint32_t)this->data_->phase1.stage_list[rotate_type][phase1_type].size());
                    }
                    printf("\n");
                }
                out_rotate_type = 0;
            }
            else if (this->is_phase2()) {
                if (solvable) {
                    //printf("\n");
                    printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                    printf("rotate_type = %u\n", (uint32_t)this->data_->phase2.rotate_type);
                    printf("phase1_type = %u\n", (uint32_t)this->data_->phase2.phase1_type);
                    printf("depth_limit = %u\n", (uint32_t)this->data_->phase2.depth_limit);
                    printf("index = %u\n", (uint32_t)(this->data_->phase2.index + 1));
                    printf("next.size() = %u\n", (uint32_t)cur_stages.size());
                    if (solvable) {
                        printf("move_path.size() = %u\n", (uint32_t)this->move_path_.size());
                    }
                    printf("\n");
                }
            }
        }

        return solvable;
    }

    bool call_phase2_search(size_type depth, size_type rotate_type, size_type satisfy_mask,
                            const stage_type & stage, phase2_callback & phase2_search) {
        size_type reached_mask = 0;
        size_type mask = 1;
        size_type phase1_type = 0;
        while (satisfy_mask != 0) {
            if ((satisfy_mask & mask) == mask) {
                // record min-move phrase1 stage
                this->data_->phase1.stage_list[rotate_type][phase1_type].push_back(stage);

                if (this->data_->phase1.min_depth[rotate_type][phase1_type] != -1) {
                    assert(this->data_->phase1.max_depth[rotate_type][phase1_type] != -1);
                    if ((int)depth >= this->data_->phase1.max_depth[rotate_type][phase1_type]) {
                        reached_mask |= mask;
                    }
                }
                else {
                    if (this->data_->phase1.has_solution[rotate_type] == 0) {
                        this->data_->phase1.has_solution[rotate_type] = 1;
                        // Update the depth limit
                        this->data_->phase1.depth_limit[rotate_type] = std::min(
                            std::max(depth + kMaxSlideDepth, kMinSearchDepth), kMaxSearchDepth);
                    }
                    this->data_->phase1.min_depth[rotate_type][phase1_type] = (int)depth;
                    this->data_->phase1.max_depth[rotate_type][phase1_type] = (int)(depth + kSlideDepth);
                }

                // call phase2_search()
                if (phase2_search) {
                    bool phase2_solvable = phase2_search(rotate_type, phase1_type, stage);
                }
            }
            phase1_type++;
            if (phase1_type >= MAX_PHASE1_TYPE)
                break;
            satisfy_mask &= ~mask;
            mask <<= 1;
        }

        return ((reached_mask & 0x0F) == 0x0F);
    }

    bool bitset_solve(size_type & out_rotate_type, phase2_callback & phase2_search) {
        size_u satisfy_result = this->is_satisfy(this->player_board_, this->target_board_, this->target_len_);
        if (satisfy_result.low != 0) {
            out_rotate_type = satisfy_result.high;
            return true;
        }

        bool solvable = false;
        size_type depth = 0;

        Position empty;
        bool found_empty = this->find_empty(this->player_board_, empty);
        if (found_empty) {
            typedef SparseBitset<Board<BoardX, BoardY>, 3, BoardX * BoardY, 2> bitset_type;
            bitset_type visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = uint8_t(-1);
            start.rotate_type = 0;
            start.board = this->player_board_;
            visited.append(start.board);

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size()) {
                for (size_type i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    uint8_t empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_type total_moves = empty_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = empty_moves[n].pos;
                        if (this->is_phase2()) {
                            if (this->data_->phase2.locked[move_pos] != 0)
                                continue;
                        }

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
#if 1
                        bool insert_new = visited.try_append(next_stage.board);
                        if (!insert_new) {
                            continue;
                        }
#elif 0
                        if (visited.contains(next_stage.board)) {
                            continue;
                        }

                        visited.append(next_stage.board);
#else
                        typedef typename bitset_type::Container Container;

                        size_type last_layer;
                        Container * last_container;
                        if (visited.contains(next_stage.board, last_layer, last_container)) {
                            continue;
                        }

                        assert(last_layer >= 0 && last_layer <= BoardY);
                        assert(last_container != nullptr);
                        visited.append_new(next_stage.board, last_layer, last_container);
#endif
                        next_stage.empty = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        next_stage.move_path.push_back(move_pos);

                        next_stages.push_back(next_stage);

                        size_u satisfy_result = this->is_satisfy(next_stage.board, this->target_board_, this->target_len_);
                        size_type satisfy_mask = satisfy_result.low;
                        if (satisfy_mask != 0) {
                            solvable = true;
                            if (this->is_phase1()) {
                                size_type rotate_type = satisfy_result.high;
                                next_stage.rotate_type = (uint8_t)rotate_type;
                                bool all_reached = call_phase2_search(depth, rotate_type, satisfy_mask,
                                                                      next_stage, phase2_search);
                                if (all_reached) {
                                    exit = true;
                                }
                            }
                            else {
                                this->move_path_ = next_stage.move_path;
                                assert((depth + 1) == next_stage.move_path.size());
                                exit = true;
                                break;
                            }
                        }
                    }
                    if (!(this->is_phase1())) {
                        if (exit) {
                            break;
                        }
                    }
                }

                depth++;
                if (this->is_phase1()) {
                    printf("depth = %u\n", (uint32_t)depth);
                    printf("cur.size() = %u, next.size() = %u\n",
                           (uint32_t)(cur_stages.size()), (uint32_t)(next_stages.size()));
                    printf("visited.size() = %u\n\n", (uint32_t)(visited.size()));
                }
                else {
                    //printf(">> %u\n", (uint32_t)depth);
                }

                std::swap(cur_stages, next_stages);
                next_stages.clear();

                if (this->is_phase1()) {
                    size_type rotate_done = 0;
                    for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                        if (this->data_->phase1.depth_limit[rotate_type] != size_t(-1) &&
                            depth > this->data_->phase1.depth_limit[rotate_type]) {
                            rotate_done++;
                        }
                    }
                    if (AllowRotate) {
                        if (rotate_done >= MAX_ROTATE_TYPE)
                            exit = true;
                    }
                    else {
                        if (rotate_done >= 1)
                            exit = true;
                    }
                }
                else if (this->is_phase2()) {
                    if (depth >= this->data_->phase2.depth_limit) {
                        exit = true;
                    }
                }

                if (exit) {
                    break;
                }
            }

            this->map_used_ = visited.size();

            if (this->is_phase1()) {
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                    for (size_type phase1_type = 0; phase1_type < MAX_PHASE1_TYPE; phase1_type++) {
                        printf("rotate_type = %u, phase1_type = %u, min_depth = %d, max_depth = %d, stage.size() = %u\n",
                                (uint32_t)rotate_type,
                                (uint32_t)phase1_type,
                                this->data_->phase1.min_depth[rotate_type][phase1_type],
                                this->data_->phase1.max_depth[rotate_type][phase1_type],
                                (uint32_t)this->data_->phase1.stage_list[rotate_type][phase1_type].size());
                    }
                    printf("\n");
                }
                out_rotate_type = 0;
            }
            else if (this->is_phase2()) {
                //printf("\n");
                if (solvable) {
                    printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                    printf("rotate_type = %u\n", (uint32_t)this->data_->phase2.rotate_type);
                    printf("phase1_type = %u\n", (uint32_t)this->data_->phase2.phase1_type);
                    printf("depth_limit = %u\n", (uint32_t)this->data_->phase2.depth_limit);
                    printf("index = %u\n", (uint32_t)(this->data_->phase2.index + 1));
                    printf("next.size() = %u\n", (uint32_t)cur_stages.size());
                    if (solvable) {
                        printf("move_path.size() = %u\n", (uint32_t)this->move_path_.size());
                    }
                    printf("\n");
                }
            }

            if (this->is_phase1()) {
                visited.display_trie_info();
            }
        }

        return solvable;
    }
};

} // namespace v1
} // namespace MagicBlock
