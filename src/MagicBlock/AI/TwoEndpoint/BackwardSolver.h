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

#include "MagicBlock/AI/TwoEndpoint/BaseBWSolver.h"

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
class BackwardSolver : public BaseBWSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack>
{
public:
    typedef BaseBWSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack>   base_type;
    typedef BackwardSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack> this_type;

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

    typedef SparseBitset<Board<BoardX, BoardY>, 3, BoardX * BoardY, 2> bitset_type;

private:
    bitset_type visited_;

    std::vector<stage_type> cur_stages_;
    std::vector<stage_type> next_stages_;

public:
    BackwardSolver(shared_data_type * data) : base_type(data) {
        this->init();
    }

    ~BackwardSolver() {
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

    void respawn() {
        this->clear();
        this->visited_.create_root();
    }

    void clear() {
        this->visited_.destroy();
        this->cur_stages_.clear();
        this->next_stages_.clear();
    }

    void clear_prev_depth() {
        std::swap(this->cur_stages_, this->next_stages_);
        this->next_stages_.clear();
    }

    bool find_stage_in_list(const Value128 & target_value, stage_type & target_stage) {
        for (size_type i = 0; i < this->cur_stages_.size(); i++) {
            const stage_type & stage = this->cur_stages_[i];
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

    int bitset_solve(size_type depth, size_type max_depth) {
        int result = 0;
        if (depth == 0) {
            for (size_type i = 0; i < this->target_len_; i++) {
                std::vector<Position> unknown_list;
                this->find_all_colors(this->player_board_[i], Color::Unknown, unknown_list);

                for (size_type n = 0; n < unknown_list.size(); n++) {
                    Position empty_pos = unknown_list[n];
                    assert(this->player_board_[i].cells[empty_pos] == Color::Unknown);
                    // Setting empty color
                    this->player_board_[i].cells[empty_pos] = Color::Empty;

                    stage_type start;
                    start.empty_pos = empty_pos;
                    start.last_dir = uint8_t(-1);
                    start.rotate_type = uint8_t((i & 0x03U) | (size_type(empty_pos) << 2U));
                    start.board = this->player_board_[i];

                    // Restore unknown color
                    this->player_board_[i].cells[empty_pos] = Color::Unknown;

                    bool insert_new = this->visited_.try_append(start.board);
                    if (!insert_new) {
                        continue;
                    }
                    this->cur_stages_.push_back(start);
                }
            }
        }

        // Search one depth only
        {
            bool exit = false;
            if (this->cur_stages_.size() > 0) {
                for (size_type i = 0; i < this->cur_stages_.size(); i++) {
                    const stage_type & stage = this->cur_stages_[i];

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

                        bool insert_new = this->visited_.try_append(next_stage.board);
                        if (!insert_new) {
                            continue;
                        }

                        next_stage.empty_pos = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = stage.rotate_type;
                        next_stage.move_path = stage.move_path;
                        next_stage.move_path.push_back(move_pos);

                        this->next_stages_.push_back(next_stage);
                    }
                }

                depth++;
                printf("BackwardSolver:: depth = %u\n", (uint32_t)depth);
                printf("cur.size() = %u, next.size() = %u\n",
                        (uint32_t)(this->cur_stages_.size()), (uint32_t)(this->next_stages_.size()));
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
                printf("next.size() = %u\n", (uint32_t)this->cur_stages_.size());
                printf("move_path.size() = %u\n", (uint32_t)this->best_move_path_.size());
                printf("\n");
            }

            this->visited_.display_trie_info();
        }

        return result;
    }

    int bitset_find_stage(const Value128 & target_value, stage_type & target_stage, size_type max_depth) {
        int result = 0;
        size_type depth = 0;

        for (size_type i = 0; i < this->target_len_; i++) {
            std::vector<Position> unknown_list;
            this->find_all_colors(this->player_board_[i], Color::Unknown, unknown_list);

            for (size_type n = 0; n < unknown_list.size(); n++) {
                Position empty_pos = unknown_list[n];
                assert(this->player_board_[i].cells[empty_pos] == Color::Unknown);
                // Setting empty color
                this->player_board_[i].cells[empty_pos] = Color::Empty;

                stage_type start;
                start.empty_pos = empty_pos;
                start.last_dir = uint8_t(-1);
                start.rotate_type = uint8_t((i & 0x03U) | (size_type(empty_pos) << 2U));
                start.board = this->player_board_[i];

                // Restore unknown color
                this->player_board_[i].cells[empty_pos] = Color::Unknown;

                bool insert_new = this->visited_.try_append(start.board);
                if (!insert_new) {
                    continue;
                }

                Value128 board_value = start.board.value128();
                if (board_value == target_value) {
                    target_stage = start;
                    return 1;
                }

                this->cur_stages_.push_back(start);
            }
        }

        bool exit = false;
        while (this->cur_stages_.size() > 0) {
            for (size_type i = 0; i < this->cur_stages_.size(); i++) {
                const stage_type & stage = this->cur_stages_[i];

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

                    bool insert_new = this->visited_.try_append(next_stage.board);
                    if (!insert_new) {
                        continue;
                    }

                    next_stage.empty_pos = move_pos;
                    next_stage.last_dir = cur_dir;
                    next_stage.rotate_type = stage.rotate_type;
                    next_stage.move_path = stage.move_path;
                    next_stage.move_path.push_back(move_pos);

                    Value128 board_value = next_stage.board.value128();
                    if (board_value == target_value) {
                        result = 1;
                        exit = true;
                        this->rotate_type_ = next_stage.rotate_type;
                        target_stage = next_stage;
                        this->best_move_path_ = next_stage.move_path;
                        break;
                    }

                    this->next_stages_.push_back(next_stage);
                }

                if (exit) {
                    break;
                }
            }

            depth++;
#if 0
            printf("BackwardSolver:: depth = %u\n", (uint32_t)depth);
            printf("cur.size() = %u, next.size() = %u\n",
                    (uint32_t)(this->cur_stages_.size()), (uint32_t)(this->next_stages_.size()));
            printf("visited.size() = %u\n\n", (uint32_t)(this->visited_.size()));
#endif
            std::swap(this->cur_stages_, this->next_stages_);
            this->next_stages_.clear();

            if (result != 1 && (depth >= max_depth || this->cur_stages_.size() == 0)) {
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
            printf("next.size() = %u\n", (uint32_t)this->cur_stages_.size());
            printf("move_path.size() = %u\n", (uint32_t)this->move_path_.size());
            printf("\n");
#endif
        }

        this->visited_.display_trie_info();
        return result;
    }
};

} // namespace TwoEndpoint
} // namespace AI
} // namespace MagicBlock
