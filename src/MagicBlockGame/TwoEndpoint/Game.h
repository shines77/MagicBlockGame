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

#include "internal/BaseGame.h"
#include "TwoEndpoint/ForwardSolver.h"
#include "TwoEndpoint/BackwardSolver.h"

#include "Constant.h"
#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "SharedData.h"
#include "ErrorCode.h"
#include "Utils.h"
#include "StopWatch.h"

//
// Two endpoint / two leg algorithm
//
namespace MagicBlock {
namespace TwoEndpoint {

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
    typedef typename base_type::phase2_callback     phase2_callback;

    static const size_type kSingelColorNums = (BoardX * BoardY - 1) / (Color::Last - 1);

    static const ptrdiff_t kStartX = (BoardX - TargetX) / 2;
    static const ptrdiff_t kStartY = (BoardY - TargetY) / 2;

    typedef ForwardSolver <BoardX, BoardY, TargetX, TargetY, false,       SolverType::Full,         phase2_callback>  TForwardSolver;
    typedef BackwardSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, SolverType::BackwardFull, phase2_callback>  TBackwardSolver;

private:
    //

public:
    Game() : base_type() {
        this->init();
    }

    virtual ~Game() {
        this->destory();
    }

    void destory() {
        // TODO:
    }

    bool solve(size_type max_forward_depth, size_type max_backward_depth) {
        return false;
    }

    bool bitset_solve(size_type max_forward_depth, size_type max_backward_depth) {
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

            TForwardSolver  forward_solver(&this->data_);
            TBackwardSolver backward_solver(&this->data_);

            int forward_status, backward_status;
            size_type forward_depth = 0;
            size_type backward_depth = 0;

            sw.start();
            while (forward_depth < max_forward_depth &&
                   backward_depth < max_backward_depth) {
                forward_status  = forward_solver.bitset_solve(forward_depth, max_forward_depth);
                backward_status = backward_solver.bitset_solve(backward_depth, max_backward_depth);
            }
            sw.stop();

            if (solvable) {
                double elapsed_time = sw.getElapsedMillisec();
                printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
                //System::pause();

                this->move_path_ = forward_solver.getMovePath();
                size_type total_steps = forward_solver.getMovePath().size() +
                                        backward_solver.getMovePath().size();
                printf("Forward moves: %u, Backward moves: %u, Total moves: %u\n\n",
                        (uint32_t)forward_solver.getMovePath().size(),
                        (uint32_t)backward_solver.getMovePath().size(),
                        (uint32_t)total_steps);

                if (total_steps < this->min_steps_) {
                    this->map_used_ = forward_solver.getMapUsed();
                    this->min_steps_ = total_steps;
                    this->best_move_path_ = forward_solver.getMovePath();
                    for (auto iter : this->move_path_) {
                        this->best_move_path_.push_back(iter);
                    }
                    printf("Total moves: %u\n\n", (uint32_t)this->best_move_path_.size());
                }

                printf("min_steps: %u\n", (uint32_t)this->min_steps_);
                printf("Total moves: %u\n", (uint32_t)this->best_move_path_.size());
                printf("\n");

                if (this->min_steps_ != size_type(-1) || this->best_move_path_.size() > 0) {
                    solvable = true;
                    if (this->translateMovePath(this->best_move_path_)) {
                        this->displayAnswer(this->answer_);
                    }
                }
            }
        }

        return solvable;
    }
};

} // namespace TwoEndpoint
} // namespace MagicBlock
