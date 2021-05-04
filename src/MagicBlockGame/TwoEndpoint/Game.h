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

    typedef typename TForwardSolver::bitset_type::Container     ForwardContainer;
    typedef typename TBackwardSolver::bitset_type::Container    BackwardContainer;

private:
    int bit_segments_[8];

    Board<BoardX, BoardY> answer_board_;

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

    bool is_coincident(int fw_value, int bw_value) const {
        //
        return false;
    }

    bool travel_forward_visited(ForwardContainer * fw_container, BackwardContainer * bw_container, size_type layer) {
        assert(fw_container != nullptr);
        assert(bw_container != nullptr);
        bool found = false;
        for (size_type i = fw_container->begin(); i < fw_container->end(); fw_container->next(i)) {
            int fw_value = fw_container->indexOf(i);
            assert(fw_value != -1);
            ForwardContainer * fw_child = fw_container->valueOf(i);
            assert (fw_child != nullptr);
            for (size_type j = bw_container->begin(); j < bw_container->end(); bw_container->next(j)) {
                int bw_value = bw_container->indexOf(j);
                assert(bw_value != -1);
                BackwardContainer * bw_child = bw_container->valueOf(j);
                assert (bw_child != nullptr);
                // overlapped, coincident
                bool overlapped = this->is_coincident(fw_value, bw_value);
                if (overlapped) {
                    this->bit_segments_[layer] = fw_container->indexOf(i);
                    if (!fw_child->isLeaf()) {
                        return this->travel_forward_visited(fw_child, bw_child, layer + 1);
                    }
                    else {
                        // Got the answer
                        this->answer_board_ = this->data_.player_board;
                        return true;
                    }
                }
            }
        }
        return found;
    }

    bool find_intersection(typename TForwardSolver::bitset_type & forward_visited,
                           typename TBackwardSolver::bitset_type & backward_visited) {
        bool found = false;
        ForwardContainer * fw_container = forward_visited.root();
        if (fw_container == nullptr)
            return false;

        BackwardContainer * bw_container = backward_visited.root();
        if (bw_container == nullptr)
            return false;

        for (size_type i = fw_container->begin(); i < fw_container->end(); fw_container->next(i)) {
            int fw_value = fw_container->indexOf(i);
            assert(fw_value != -1);
            ForwardContainer * fw_child = fw_container->valueOf(i);
            assert(fw_child != nullptr);
            for (size_type j = bw_container->begin(); j < bw_container->end(); bw_container->next(j)) {
                int bw_value = bw_container->indexOf(j);
                assert(bw_value != -1);
                BackwardContainer * bw_child = bw_container->valueOf(j);
                assert(bw_child != nullptr);
                // overlapped, coincident
                bool overlapped = this->is_coincident(fw_value, bw_value);
                if (overlapped) {
                    this->bit_segments_[0] = fw_container->indexOf(i);
                    found = this->travel_forward_visited(fw_child, bw_child, 1);
                    if (found) {
                        return true;
                    }
                }
            }
        }

        return found;
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
            while (forward_depth < max_forward_depth && backward_depth < max_backward_depth) {
                printf("-------------------------------------------------\n\n");
                forward_status  = forward_solver.bitset_solve(forward_depth, max_forward_depth);
                printf("------------------------\n\n");
                backward_status = backward_solver.bitset_solve(backward_depth, max_backward_depth);

                if (forward_status == 1 && backward_status == 1) {
                    solvable = true;
                    break;
                }

                Board<BoardX, BoardY> answer;
                bool found = this->find_intersection(forward_solver.visited(), backward_solver.visited());
                if (found) {
                    // Got a answer
                }

                forward_depth++;
                backward_depth++;
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
