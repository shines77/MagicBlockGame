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
#include <cmath>
#include <cstring>
#include <vector>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <exception>
#include <stdexcept>
#include <algorithm>    // For std::swap(), until C++11. std::min()
#include <utility>      // For std::swap(), since C++11

#include "MagicBlock/AI/internal/BaseSolver.h"

#include "MagicBlock/AI/Constant.h"
#include "MagicBlock/AI/Color.h"
#include "MagicBlock/AI/Move.h"
#include "MagicBlock/AI/CanMoves.h"
#include "MagicBlock/AI/Value128.h"
#include "MagicBlock/AI/Board.h"
#include "MagicBlock/AI/Stage.h"
#include "MagicBlock/AI/SharedData.h"
#include "MagicBlock/AI/SparseBitset.h"
#include "MagicBlock/AI/Utils.h"

namespace MagicBlock {
namespace AI {
namespace TwoEndpoint {

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t TargetX, std::size_t TargetY,
          bool AllowRotate, std::size_t N_SolverType,
          typename Phase2CallBack>
class ForwardSolver : public internal::BaseSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack>
{
public:
    typedef internal::BaseSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack> base_type;
    typedef ForwardSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack>        this_type;

    typedef typename base_type::size_type           size_type;
    typedef typename base_type::ssize_type          ssize_type;

    typedef typename base_type::shared_data_type    shared_data_type;
    typedef typename base_type::stage_type          stage_type;
    typedef typename base_type::stage_info_t        stage_info_t;
    typedef typename base_type::can_moves_t         can_moves_t;
    typedef typename base_type::can_move_list_t     can_move_list_t;
    typedef typename base_type::player_board_t      player_board_t;
    typedef typename base_type::target_board_t      target_board_t;
    typedef typename base_type::phase2_callback     phase2_callback;

    static const size_type BoardSize = BoardX * BoardY;
    static const size_type kSingelColorNums = (BoardSize - 1) / (Color::Last - 1);

    static const ptrdiff_t kStartX = (BoardX - TargetX) / 2;
    static const ptrdiff_t kStartY = (BoardY - TargetY) / 2;

    typedef SparseBitset<Board<BoardX, BoardY>, 3, BoardX * BoardY>     bitset_type;
    typedef std::set<Value128>                                          stdset_type;
    typedef std::unordered_set<Value128, Value128_Hash>                 stdset_type_;
    typedef std::unordered_set<Value128, Value128_Hash>                 std_hashset_t;

private:
    bitset_type visited_;
    stdset_type visited_set_;

    std::vector<stage_type> curr_stages_;
    std::vector<stage_type> next_stages_;

    void init() {
        assert(this->data_ != nullptr);

        this->player_board_ = this->data_->player_board;
        for (size_type i = 0; i < MAX_ROTATE_TYPE; i++) {
            this->target_board_[i] = this->data_->target_board[i];
        }
        if (AllowRotate)
            this->target_len_ = this->data_->target_len;
        else
            this->target_len_ = 1;

        this->count_target_color_nums(this->target_board_[0]);
    }

public:
    ForwardSolver(shared_data_type * data) : base_type(data) {
        this->init();
    }

    ~ForwardSolver() {
        this->destory();
    }

    void destory() {
        // TODO:
    }

    bitset_type & visited() {
        return this->visited_;
    }

    const bitset_type & visited() const {
        return this->visited_;
    }

    stdset_type & visited_set() {
        return this->visited_set_;
    }

    const stdset_type & visited_set() const {
        return this->visited_set_;
    }

    std::vector<stage_type> & curr_stages() {
        return this->curr_stages_;
    }

    const std::vector<stage_type> & curr_stages() const {
        return this->curr_stages_;
    }

    std::vector<stage_type> & next_stages() {
        return this->next_stages_;
    }

    const std::vector<stage_type> & next_stages() const {
        return this->next_stages_;
    }

    void respawn() {
        this->clear();
        this->visited_.create_root();
    }

    void clear() {
        this->visited_.destroy();
        this->curr_stages_.clear();
        this->next_stages_.clear();
    }

    size_type calc_next_capacity() const {
        size_type curr_size = this->curr_stages_.size();
        size_type next_size = this->next_stages_.size();
        double growth_rate;
        if (curr_size != 0)
            growth_rate = (double)next_size / curr_size;
        else
            growth_rate = 0.0;
        growth_rate += 0.6;
        size_type next_capacity = (size_type)std::floor(growth_rate * next_size);
        return next_capacity;
    }

    void clear_prev_depth() {
        size_type next_capacity = calc_next_capacity();
        std::swap(this->curr_stages_, this->next_stages_);
        this->next_stages_.clear();
        this->next_stages_.reserve(next_capacity);
    }

    bool find_stage_in_list(const Value128 & target_value, stage_type & target_stage) {
        for (size_type i = 0; i < this->curr_stages_.size(); i++) {
            const stage_type & stage = this->curr_stages_[i];
            const Value128 & value = stage.board.value128();
            if (value == target_value) {
                target_stage = stage;
                return true;
            }
        }
        for (size_type i = 0; i < this->next_stages_.size(); i++) {
            const stage_type & stage = this->next_stages_[i];
            const Value128 & value = stage.board.value128();
            if (value == target_value) {
                target_stage = stage;
                return true;
            }
        }
        return false;
    }

    int stdset_solve(size_type depth, size_type max_depth) {
        int result = 0;
        if (depth == 0) {
            size_u satisfy_result = this->is_satisfy(this->player_board_,
                                                     this->target_board_,
                                                     this->target_len_);
            if (satisfy_result.low != 0) {
                return 1;
            }

            Position empty;
            bool found_empty = this->find_empty(this->player_board_, empty);
            if (found_empty) {
                stage_type start(this->player_board_);
                start.empty_pos = empty;
                start.last_dir = uint8_t(-1);
                start.rotate_type = 0;

                Value128 board_value = start.board.value128();
                this->visited_set_.insert(board_value);
                this->curr_stages_.push_back(start);
            }
        }

        // Search one depth only
        {
            bool exit = false;
            if (this->curr_stages_.size() > 0) {
                for (size_type i = 0; i < this->curr_stages_.size(); i++) {
                    const stage_type & stage = this->curr_stages_[i];

                    uint8_t empty_pos = stage.empty_pos;
                    const can_move_list_t & can_moves = this->data_->can_moves[empty_pos];
                    size_type total_moves = can_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = can_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = can_moves[n].pos;

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);

                        Value128 board_value = next_stage.board.value128();
                        if (this->visited_set_.count(board_value) > 0)
                            continue;

                        this->visited_set_.insert(board_value);

                        next_stage.empty_pos = move_pos;
                        next_stage.last_dir = Dir::opp_dir(cur_dir);
                        //next_stage.rotate_type = 0;
                        next_stage.move_seq = stage.move_seq;
                        next_stage.move_seq.push_back(cur_dir);

                        this->next_stages_.push_back(std::move(next_stage));
                    }
                }

                depth++;
                printf("ForwardSolver::  depth = %u\n", (uint32_t)depth);
                printf("cur.size() = %u, next.size() = %u\n",
                        (uint32_t)(this->curr_stages_.size()), (uint32_t)(this->next_stages_.size()));
                printf("visited.size() = %u\n\n", (uint32_t)(this->visited_set_.size()));

                if (depth >= max_depth) {
                    exit = true;
                    result = -1;
                }
                (void)exit;
            }

            this->map_used_ = this->visited_set_.size();

            if (result == 1) {
                printf("Solvable: %s\n\n", ((result == 1) ? "true" : "false"));
                printf("next.size() = %u\n", (uint32_t)this->curr_stages_.size());
                printf("move_seq.size() = %u\n", (uint32_t)this->move_seq_.size());
                printf("\n");
            }
        }

        return result;
    }

    int bitset_solve(size_type depth, size_type max_depth) {
        int result = 0;
        if (depth == 0) {
            size_u satisfy_result = this->is_satisfy(this->player_board_,
                                                     this->target_board_,
                                                     this->target_len_);
            if (satisfy_result.low != 0) {
                return 1;
            }

            Position empty;
            bool found_empty = this->find_empty(this->player_board_, empty);
            if (found_empty) {
                stage_type start;
                start.empty_pos = empty;
                start.last_dir = uint8_t(-1);
                start.rotate_type = 0;
                start.board = this->player_board_;

                this->visited_.insert(start.board);
                this->curr_stages_.push_back(start);
            }
        }

        // Search one depth only
        {
            bool exit = false;
            if (this->curr_stages_.size() > 0) {
                for (size_type i = 0; i < this->curr_stages_.size(); i++) {
                    stage_type & stage = this->curr_stages_[i];

                    uint8_t empty_pos = stage.empty_pos;
                    const can_move_list_t & can_moves = this->data_->can_moves[empty_pos];
                    size_type total_moves = can_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = can_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = can_moves[n].pos;
#if STAGES_USE_EMPLACE_PUSH
                        std::swap(stage.board.cells[empty_pos], stage.board.cells[move_pos]);

                        bool insert_new = this->visited_.try_insert(stage.board);
                        if (!insert_new) {
                            std::swap(stage.board.cells[empty_pos], stage.board.cells[move_pos]);
                            continue;
                        }

                        this->next_stages_.emplace_back(stage.board, move_pos, cur_dir, stage.move_seq);

                        std::swap(stage.board.cells[empty_pos], stage.board.cells[move_pos]);
#else
                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);

                        bool insert_new = this->visited_.try_insert(next_stage.board);
                        if (!insert_new) {
                            continue;
                        }

                        next_stage.empty_pos = move_pos;
                        next_stage.last_dir = Dir::opp_dir(cur_dir);
                        //next_stage.rotate_type = 0;
                        next_stage.move_seq = stage.move_seq;
                        next_stage.move_seq.push_back(cur_dir);

                        this->next_stages_.push_back(std::move(next_stage));
#endif // STAGES_USE_EMPLACE_PUSH
                    }
                }

                depth++;
                printf("ForwardSolver::  depth = %u\n", (uint32_t)depth);
                printf("cur.size() = %u, next.size() = %u\n",
                        (uint32_t)(this->curr_stages_.size()), (uint32_t)(this->next_stages_.size()));
                printf("visited.size() = %u\n\n", (uint32_t)(this->visited_.size()));

                if (depth >= max_depth) {
                    exit = true;
                    result = -1;
                }
                (void)exit;
            }

            this->map_used_ = this->visited_.size();

            if (result == 1) {
                printf("Solvable: %s\n\n", ((result == 1) ? "true" : "false"));
                printf("next.size() = %u\n", (uint32_t)this->curr_stages_.size());
                printf("move_seq.size() = %u\n", (uint32_t)this->move_seq_.size());
                printf("\n");
            }

            this->visited_.display_trie_info();
        }

        return result;
    }

    int bitset_find_stage(const Value128 & target_value, stage_type & target_stage, size_type max_depth) {
        size_u satisfy_result = this->is_satisfy(this->player_board_,
                                                 this->target_board_,
                                                 this->target_len_);
        if (satisfy_result.low != 0) {
            return 1;
        }

        int result = 0;
        size_type depth = 0;

        Position empty;
        bool found_empty = this->find_empty(this->player_board_, empty);
        if (found_empty) {
            stage_type start;
            start.empty_pos = empty;
            start.last_dir = uint8_t(-1);
            start.rotate_type = 0;
            start.board = this->player_board_;

            this->visited_.insert(start.board);
            this->curr_stages_.push_back(start);

            bool exit = false;
            while (this->curr_stages_.size() > 0) {
                for (size_type i = 0; i < this->curr_stages_.size(); i++) {
                    const stage_type & stage = this->curr_stages_[i];

                    uint8_t empty_pos = stage.empty_pos;
                    const can_move_list_t & can_moves = this->data_->can_moves[empty_pos];
                    size_type total_moves = can_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = can_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = can_moves[n].pos;

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);

                        bool insert_new = this->visited_.try_insert(next_stage.board);
                        if (!insert_new) {
                            continue;
                        }

                        next_stage.empty_pos = move_pos;
                        next_stage.last_dir = Dir::opp_dir(cur_dir);
                        next_stage.rotate_type = 0;
                        next_stage.move_seq = stage.move_seq;
                        next_stage.move_seq.push_back(cur_dir);

                        Value128 board_value = next_stage.board.value128();
                        if (board_value == target_value) {
                            result = 1;
                            exit = true;
                            target_stage = next_stage;
                            this->move_seq_ = next_stage.move_seq;
                            break;
                        }

                        this->next_stages_.push_back(std::move(next_stage));
                    }

                    if (exit) {
                        break;
                    }
                }

                depth++;
#if 0
                printf("ForwardSolver::  depth = %u\n", (uint32_t)depth);
                printf("cur.size() = %u, next.size() = %u\n",
                        (uint32_t)(this->curr_stages_.size()), (uint32_t)(this->next_stages_.size()));
                printf("visited.size() = %u\n\n", (uint32_t)(this->visited_.size()));
#endif
                size_type next_capacity = calc_next_capacity();
                std::swap(this->curr_stages_, this->next_stages_);
                this->next_stages_.clear();
                this->next_stages_.reserve(next_capacity);

                if (result != 1 && depth >= max_depth) {
                    result = -1;
                    exit = true;
                }

                if (exit) {
                    break;
                }
            }

            this->map_used_ = this->visited_.size();

            if (result == 1) {
#if 0
                printf("Solvable: %s\n\n", ((result == 1) ? "true" : "false"));
                printf("next.size() = %u\n", (uint32_t)this->curr_stages_.size());
                printf("move_seq.size() = %u\n", (uint32_t)this->move_seq_.size());
                printf("\n");
#endif
            }

            this->visited_.display_trie_info();
        }

        return result;
    }
};

} // namespace TwoEndpoint
} // namespace AI
} // namespace MagicBlock
