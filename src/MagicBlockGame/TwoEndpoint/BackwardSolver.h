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
namespace TwoEndpoint {

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t TargetX, std::size_t TargetY,
          bool AllowRotate, std::size_t N_SolverType,
          typename Phase2CallBack>
class BackwardSolver : public internal::BaseSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack>
{
public:
    typedef internal::BaseSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack> base_type;
    typedef BackwardSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack>       this_type;

    typedef typename base_type::size_type           size_type;
    typedef typename base_type::ssize_type          ssize_type;

    typedef typename base_type::shared_data_type    shared_data_type;
    typedef typename base_type::stage_type          stage_type;
    typedef typename base_type::phase2_callback     phase2_callback;

    static const size_type kSingelColorNums = (BoardX * BoardY - 1) / (Color::Last - 1);

    static const ptrdiff_t kStartX = (BoardX - TargetX) / 2;
    static const ptrdiff_t kStartY = (BoardY - TargetY) / 2;

    typedef SparseBitset<Board<BoardX, BoardY>, 3, BoardX * BoardY, 2> bitset_type;

#ifdef NDEBUG
    static const size_type kDefaultSearchDepthLimit = 30;
#else
    static const size_type kDefaultSearchDepthLimit = 22;
#endif

private:
    bitset_type visited;

    std::vector<stage_type> cur_stages;
    std::vector<stage_type> next_stages;

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

        this->data_->phase1.init(kDefaultSearchDepthLimit);
    }

public:
    BackwardSolver(shared_data_type * data) : base_type(data) {
        this->init();
    }

    virtual ~BackwardSolver() {
        this->destory();
    }

    void destory() {
        // TODO:
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
                start.empty = empty;
                start.last_dir = uint8_t(-1);
                start.rotate_type = 0;
                start.board = this->player_board_;

                this->visited.append(start.board);
                this->cur_stages.push_back(start);
            }
        }

        // Search one depth only
        {
            bool exit = false;
            if (this->cur_stages.size() > 0) {
                for (size_type i = 0; i < this->cur_stages.size(); i++) {
                    const stage_type & stage = this->cur_stages[i];

                    uint8_t empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_type total_moves = empty_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = empty_moves[n].pos;

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);

                        bool insert_new = this->visited.try_append(next_stage.board);
                        if (!insert_new)
                            continue;

                        next_stage.empty = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        Position next_move(stage.empty);
                        next_stage.move_path.push_back(next_move);

                        this->next_stages.push_back(next_stage);

                        size_u satisfy_result = this->is_satisfy(next_stage.board, this->target_board_, this->target_len_);
                        size_type satisfy_mask = satisfy_result.low;
                        if (satisfy_mask != 0) {
                            result = 1;

                            this->move_path_ = next_stage.move_path;
                            assert((depth + 1) == next_stage.move_path.size());
                            break;
                        }
                    }

                    if (result == 1) {
                        break;
                    }
                }

                depth++;
                printf("depth = %u\n", (uint32_t)depth);
                printf("cur.size() = %u, next.size() = %u\n",
                        (uint32_t)(this->cur_stages.size()), (uint32_t)(this->next_stages.size()));
                printf("visited.size() = %u\n\n", (uint32_t)(this->visited.size()));

                std::swap(this->cur_stages, this->next_stages);
                this->next_stages.clear();

                if (result != 1 && depth >= max_depth) {
                    exit = true;
                    result = -1;
                }
            }

            this->map_used_ = this->visited.size();

            if (result == 1) {
                printf("Solvable: %s\n\n", ((result == 1) ? "true" : "false"));
                printf("next.size() = %u\n", (uint32_t)this->cur_stages.size());
                printf("move_path.size() = %u\n", (uint32_t)this->move_path_.size());
                printf("\n");
            }

            this->visited.display_trie_info();
        }

        return result;
    }
};

} // namespace TwoEndpoint
} // namespace MagicBlock
