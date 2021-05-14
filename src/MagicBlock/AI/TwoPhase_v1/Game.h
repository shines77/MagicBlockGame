#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <set>
#include <exception>
#include <stdexcept>
#include <functional>
#include <algorithm>    // For std::swap(), until C++11
#include <utility>      // For std::swap(), since C++11
#include <climits>      // For std::numeric_limits<T>

#include "MagicBlock/AI/internal/BaseGame.h"

#include "MagicBlock/AI/Constant.h"
#include "MagicBlock/AI/Color.h"
#include "MagicBlock/AI/Move.h"
#include "MagicBlock/AI/Board.h"
#include "MagicBlock/AI/SharedData.h"
#include "MagicBlock/AI/ErrorCode.h"
#include "MagicBlock/AI/TwoPhase_v1/Solver.h"
#include "MagicBlock/AI/Utils.h"
#include "MagicBlock/AI/StopWatch.h"

namespace MagicBlock {
namespace AI {
namespace TwoPhase_v1 {

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t TargetX, std::size_t TargetY,
          bool AllowRotate = true>
class Game : public internal::BaseGame<BoardX, BoardY, TargetX, TargetY, AllowRotate>
{
public:
    typedef internal::BaseGame<BoardX, BoardY, TargetX, TargetY, AllowRotate>   base_type;
    typedef Game<BoardX, BoardY, TargetX, TargetY, AllowRotate>                 this_type;

    typedef typename base_type::size_type           size_type;
    typedef typename base_type::ssize_type          ssize_type;

    typedef typename base_type::shared_data_type    shared_data_type;
    typedef typename base_type::stage_type          stage_type;
    typedef typename base_type::player_board_t      player_board_t;
    typedef typename base_type::target_board_t      target_board_t;
    typedef typename base_type::phase2_callback     phase2_callback;

    typedef Solver<BoardX, BoardY, TargetX, TargetY, AllowRotate, SolverType::Phase1_123, phase2_callback>  Phase1Solver;
    typedef Solver<BoardX, BoardY, TargetX, TargetY, false,       SolverType::Phase2,     phase2_callback>  Phase2Solver;

    typedef Solver<BoardX, BoardY - 2, TargetX, TargetY - 1, false, SolverType::Phase2_Compact, phase2_callback>  CompactPhase2Solver;

    static const size_type BoardSize = BoardX * BoardY;
    static const size_type kSingelColorNums = (BoardSize - 1) / (Color::Last - 1);

    static const ptrdiff_t kStartX = (BoardX - TargetX) / 2;
    static const ptrdiff_t kStartY = (BoardY - TargetY) / 2;

private:
    //

public:
    Game() : base_type() {
    }

    ~Game() {
        this->destory();
    }

    void destory() {
        // TODO:
    }

    bool solve() {
        if (this->is_satisfy(this->data_.player_board,
                             this->data_.target_board,
                             this->data_.target_len) != 0) {
            return true;
        }

        bool solvable = false;
        size_type out_rotate_type = 0;

        Position empty;
        bool found_empty = this->find_empty(this->data_.player_board, empty);
        if (found_empty) {
            jtest::StopWatch sw;

            Phase1Solver solver(&this->data_);
            sw.start();
            bool phase1_solvable = solver.solve(out_rotate_type);
            sw.stop();

            if (phase1_solvable) {
                double elapsed_time = sw.getElapsedMillisec();
                printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
                //System::pause();

                for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                    for (size_type phase1_type = 0; phase1_type < MAX_PHASE1_TYPE; phase1_type++) {
                        const std::vector<stage_type> & stage_list = this->data_.phase1.stage_list[rotate_type][phase1_type];
                        size_type totalStage = stage_list.size();
                        for (size_type n = 0; n < totalStage; n++) {
                            size_type move_path_size = stage_list[n].move_path.size();
                            if (this->min_steps_ > move_path_size) {
                                this->data_.phase2.depth_limit = std::min(ssize_type(MAX_PHASE2_DEPTH),
                                    std::max(ssize_type(this->min_steps_ - 1 - move_path_size), ssize_type(1)));
                            }
                            else {
                                continue;
                            }

                            this->data_.phase2.rotate_type = rotate_type;
                            this->data_.phase2.phase1_type = phase1_type;
                            this->data_.phase2.index = n;

                            Phase2Solver phase2_solver(&this->data_);
                            phase2_solver.setPlayerBoard(stage_list[n].board);
                            phase2_solver.setRotateType(stage_list[n].rotate_type);

                            bool phase2_solvable = phase2_solver.solve(out_rotate_type);
                            if (phase2_solvable) {
                                solvable = true;
                                this->move_path_ = phase2_solver.getMovePath();
                                size_type total_steps = move_path_size + this->move_path_.size();
                                printf("Phase1 moves: %u, Phase2 moves: %u, Total moves: %u\n\n",
                                       (uint32_t)move_path_size,
                                       (uint32_t)this->move_path_.size(),
                                       (uint32_t)total_steps);

                                if (total_steps < this->min_steps_) {
                                    this->map_used_ = phase2_solver.getMapUsed();
                                    this->min_steps_ = total_steps;
                                    this->best_move_path_ = stage_list[n].move_path;
                                    for (auto iter : this->move_path_) {
                                        this->best_move_path_.push_back(iter);
                                    }
                                    //printf("Total moves: %u\n\n", (uint32_t)this->best_move_path_.size());
                                }
                            }
                        }
                    }
                }

                printf("min_steps: %u\n", (uint32_t)this->min_steps_);
                printf("Total moves: %u\n", (uint32_t)this->best_move_path_.size());
                printf("\n");

                if (this->min_steps_ != size_t(-1) || this->best_move_path_.size() > 0) {
                    solvable = true;

                    this->displayAnswerMoves(this->best_move_path_);
                }
            }
        }

        return solvable;
    }

    bool bitset_solve() {
        if (this->is_satisfy(this->data_.player_board,
                             this->data_.target_board,
                             this->data_.target_len) != 0) {
            return true;
        }

        bool solvable = false;
        size_type out_rotate_type = 0;
        phase2_callback phase_search_cb;

        Position empty;
        bool found_empty = this->find_empty(this->data_.player_board, empty);
        if (found_empty) {
            jtest::StopWatch sw;

            Phase1Solver solver(&this->data_);
            sw.start();
            bool phase1_solvable = solver.bitset_solve(out_rotate_type, phase_search_cb);
            sw.stop();

            if (phase1_solvable) {
                double elapsed_time = sw.getElapsedMillisec();
                printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
                //System::pause();

                for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                    for (size_type phase1_type = 0; phase1_type < MAX_PHASE1_TYPE; phase1_type++) {
                        const std::vector<stage_type> & stage_list = this->data_.phase1.stage_list[rotate_type][phase1_type];
                        size_type totalStage = stage_list.size();
                        for (size_type n = 0; n < totalStage; n++) {
                            size_type move_path_size = stage_list[n].move_path.size();
                            if (this->min_steps_ > move_path_size) {
                                this->data_.phase2.depth_limit = std::min(ssize_type(MAX_PHASE2_DEPTH),
                                    std::max(ssize_type(this->min_steps_ - 1 - move_path_size), ssize_type(5)));
                            }
                            else {
                                continue;
                            }

                            this->data_.phase2.rotate_type = rotate_type;
                            this->data_.phase2.phase1_type = phase1_type;
                            this->data_.phase2.index = n;

                            Phase2Solver phase2_solver(&this->data_);
                            phase2_solver.setPlayerBoard(stage_list[n].board);
                            phase2_solver.setRotateType(stage_list[n].rotate_type);

                            //bool phase2_solvable = phase2_solver.bitset_solve(out_rotate_type, phase_search_cb);
                            bool phase2_solvable = phase2_solver.solve(out_rotate_type);
                            if (phase2_solvable) {
                                solvable = true;
                                this->move_path_ = phase2_solver.getMovePath();
                                size_type total_steps = move_path_size + this->move_path_.size();
                                printf("Phase1 moves: %u, Phase2 moves: %u, Total moves: %u\n\n",
                                       (uint32_t)move_path_size,
                                       (uint32_t)this->move_path_.size(),
                                       (uint32_t)total_steps);

                                if (total_steps < this->min_steps_) {
                                    this->map_used_ = phase2_solver.getMapUsed();
                                    this->min_steps_ = total_steps;
                                    this->best_move_path_ = stage_list[n].move_path;
                                    for (auto iter : this->move_path_) {
                                        this->best_move_path_.push_back(iter);
                                    }
                                    //printf("Total moves: %u\n\n", (uint32_t)this->best_move_path_.size());
                                }
                            }
                        }
                    }
                }

                printf("min_steps: %u\n", (uint32_t)this->min_steps_);
                printf("Total moves: %u\n", (uint32_t)this->best_move_path_.size());
                printf("\n");

                if (this->min_steps_ != size_type(-1) || this->best_move_path_.size() > 0) {
                    solvable = true;

                    this->displayAnswerMoves(this->best_move_path_);
                }
            }
        }

        return solvable;
    }

    bool bitset_solve_phase2_search(size_type rotate_type, size_type phase1_type, const stage_type & stage) {
        static size_type phase2_stage_cnt = 0;
        this->data_.phase2.index = phase2_stage_cnt;
        phase2_stage_cnt++;

        size_type move_path_size = stage.move_path.size();
        if (this->min_steps_ > move_path_size) {
            this->data_.phase2.depth_limit = std::min(ssize_type(MAX_PHASE2_DEPTH),
                std::max(ssize_type(this->min_steps_ - 1 - move_path_size), ssize_type(1)));
        }
        else {
            return false;
        }

        this->data_.phase2.rotate_type = rotate_type;
        this->data_.phase2.phase1_type = phase1_type;

        Phase2Solver solver(&this->data_);
        solver.setPlayerBoard(stage.board);
        solver.setRotateType(rotate_type);

        size_type out_rotate_type = 0;
        phase2_callback dummy_phase2_search;

        bool solvable = solver.solve(out_rotate_type);
        //bool solvable = solver.bitset_solve(out_rotate_type, dummy_phase2_search);
        if (solvable) {
            this->move_path_ = solver.getMovePath();
            size_type total_steps = stage.move_path.size() + this->move_path_.size();
            printf("Phase1 moves: %u, Phase2 moves: %u, Total moves: %u\n\n",
                    (uint32_t)stage.move_path.size(),
                    (uint32_t)this->move_path_.size(),
                    (uint32_t)total_steps);

            if (total_steps < this->min_steps_) {
                this->map_used_ = solver.getMapUsed();
                this->min_steps_ = total_steps;
                this->best_move_path_ = stage.move_path;
                for (auto iter : this->move_path_) {
                    this->best_move_path_.push_back(iter);
                }
                //printf("Total moves: %u\n\n", (uint32_t)this->best_move_path_.size());
            }
        }

        return solvable;
    }

    bool bitset_solve_immediate() {
        if (this->is_satisfy(this->data_.player_board,
                             this->data_.target_board,
                             this->data_.target_len) != 0) {
            return true;
        }

        bool solvable = false;
        size_type out_rotate_type = 0;

        using namespace std::placeholders;
        phase2_callback phase_search_cb = std::bind(&Game::bitset_solve_phase2_search, this, _1, _2, _3);

        Position empty;
        bool found_empty = this->find_empty(this->data_.player_board, empty);
        if (found_empty) {
            jtest::StopWatch sw;

            Phase1Solver solver(&this->data_);
            sw.start();
            solvable = solver.bitset_solve(out_rotate_type, phase_search_cb);
            sw.stop();

            if (solvable) {
                double elapsed_time = sw.getElapsedMillisec();
                printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);

                printf("min_steps: %u\n", (uint32_t)this->min_steps_);
                printf("Total moves: %u\n", (uint32_t)this->best_move_path_.size());
                printf("\n");

                if (this->min_steps_ != size_type(-1) || this->best_move_path_.size() > 0) {
                    solvable = true;

                    this->displayAnswerMoves(this->best_move_path_);
                }
            }
        }

        return solvable;
    }
};

} // namespace TwoPhase_v1
} // namespace AI
} // namespace MagicBlock
