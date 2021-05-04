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
    int fw_segments_[8];
    int bw_segments_[8];

    Board<BoardX, BoardY> fw_answer_board_;
    Board<BoardX, BoardY> bw_answer_board_;

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
        std::uint32_t fw_value32 = (std::uint32_t)fw_value;
        std::uint32_t bw_value32 = (std::uint32_t)bw_value;
        if ((bw_value32 & 0x00000007U) == 0x00000007U) {
            if ((fw_value32 & 0x00000007U) != 0) {
                fw_value32 |= 0x00000007U;
            }
        }
        if ((bw_value32 & 0x00000038U) == 0x00000038U) {
            if ((fw_value32 & 0x00000038U) != 0) {
                fw_value32 |= 0x00000038U;
            }
        }
        if ((bw_value32 & 0x000001C0U) == 0x000001C0U) {
            if ((fw_value32 & 0x000001C0U) != 0) {
                fw_value32 |= 0x000001C0U;
            }
        }
        if ((bw_value32 & 0x00000E00U) == 0x00000E00U) {
            if ((fw_value32 & 0x00000E00U) != 0) {
                fw_value32 |= 0x00000E00U;
            }
        }
        if ((bw_value32 & 0x00007000U) == 0x00007000U) {
            if ((fw_value32 & 0x00007000U) != 0) {
                fw_value32 |= 0x00007000U;
            }
        }
        return (fw_value32 == bw_value32);
    }

    void compose_segments_to_board(Board<BoardX, BoardY> & board, const int segment_list[8], size_type segment_len) {
        size_type pos = 0;
        for (size_type segment = 0; segment < segment_len; segment++) {
            std::uint32_t value = (std::uint32_t)segment_list[segment];
            for (size_type n = 0; n < 5; n++) {
                std::uint32_t color = value & Color::Mask32;
                assert(color >= Color::Empty && color < Color::Maximum);
                assert(pos < (BoardX * BoardY));
                board.cells[pos++] = (std::uint8_t)color;
                value >>= Color::Shift32;
            }
        }
    }

    bool travel_forward_visited_leaf(ForwardContainer * fw_container, BackwardContainer * bw_container, size_type layer) {
        assert(fw_container != nullptr);
        assert(bw_container != nullptr);
        bool found = false;
        for (size_type i = fw_container->begin(); i < fw_container->end(); fw_container->next(i)) {
            int fw_value = fw_container->indexOf(i);
            assert(fw_value != -1);
            for (size_type j = bw_container->begin(); j < bw_container->end(); bw_container->next(j)) {
                int bw_value = bw_container->indexOf(j);
                assert(bw_value != -1);
                // It's overlapped ?
                bool overlapped = this->is_coincident(fw_value, bw_value);
                if (overlapped) {
                    // Record the board segment value of layer N
                    this->fw_segments_[layer] = fw_value;
                    this->bw_segments_[layer] = bw_value;

                    // Got the answer
                    this->compose_segments_to_board(this->fw_answer_board_, this->fw_segments_, layer + 1);
                    this->compose_segments_to_board(this->bw_answer_board_, this->bw_segments_, layer + 1);
                    return true;
                }
            }
        }
        return found;
    }

    bool travel_forward_visited(ForwardContainer * fw_container, BackwardContainer * bw_container, size_type layer) {
        assert(fw_container != nullptr);
        assert(bw_container != nullptr);
        bool found = false;
        for (size_type i = fw_container->begin(); i < fw_container->end(); fw_container->next(i)) {
            int fw_value = fw_container->indexOf(i);
            assert(fw_value != -1);
            for (size_type j = bw_container->begin(); j < bw_container->end(); bw_container->next(j)) {
                int bw_value = bw_container->indexOf(j);
                assert(bw_value != -1);
                // It's overlapped ?
                bool overlapped = this->is_coincident(fw_value, bw_value);
                if (overlapped) {
                    // Record the board segment value of layer N
                    this->fw_segments_[layer] = fw_value;
                    this->bw_segments_[layer] = bw_value;
                    
                    ForwardContainer * fw_child = fw_container->valueOf(i);
                    assert (fw_child != nullptr);
                    BackwardContainer * bw_child = bw_container->valueOf(j);
                    assert (bw_child != nullptr);

                    if (!fw_child->isLeaf()) {
                        // Travel the next layer if it's not a leaf container
                        return this->travel_forward_visited(fw_child, bw_child, layer + 1);
                    }
                    else {
                        // Search and compare the leaf containers
                        return this->travel_forward_visited_leaf(fw_child, bw_child, layer + 1);
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
            if (fw_value != -1) {
                for (size_type j = bw_container->begin(); j < bw_container->end(); bw_container->next(j)) {
                    int bw_value = bw_container->indexOf(j);
                    if (bw_value != -1) {
                        // It's overlapped ?
                        bool overlapped = this->is_coincident(fw_value, bw_value);
                        if (overlapped) {
                            // Record the board segment value of layer 0
                            this->fw_segments_[0] = fw_value;
                            this->bw_segments_[0] = bw_value;

                            ForwardContainer * fw_child = fw_container->valueOf(i);
                            assert(fw_child != nullptr);
                            BackwardContainer * bw_child = bw_container->valueOf(j);
                            assert(bw_child != nullptr);

                            // Travel the next layer
                            found = this->travel_forward_visited(fw_child, bw_child, 1);
                            if (found) {
                                return true;
                            }
                        }
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

        Position empty;
        bool found_empty = this->find_empty(this->data_.player_board, empty);
        if (found_empty) {
            jtest::StopWatch sw;

            TForwardSolver forward_solver(&this->data_);
            TBackwardSolver backward_solver(&this->data_);

            int forward_status, backward_status;
            size_type forward_depth = 0;
            size_type backward_depth = 0;

            printf("-----------------------------------------------\n\n");

            sw.start();
            while (forward_depth < max_forward_depth && backward_depth < max_backward_depth) {
                int iterative_type = 0;
#if 1
                if (forward_depth > 15) {
                    size_type fw_visited_size = forward_solver.visited().size();
                    size_type bw_visited_size = backward_solver.visited().size();
                    if (fw_visited_size >= bw_visited_size * 2) {
                        iterative_type = 1;
                    }
                    else if (bw_visited_size >= fw_visited_size * 2) {
                        iterative_type = 2;
                    }
                }
#endif
                if (iterative_type == 1) {
                    forward_status  = 0;
                    backward_status = backward_solver.bitset_solve(backward_depth++, max_backward_depth);
                    printf("-----------------------------------------------\n\n");
                }
                else if (iterative_type == 2) {
                    forward_status  = forward_solver.bitset_solve(forward_depth++, max_forward_depth);
                    backward_status = 0;
                    printf("-----------------------------------------------\n\n");
                }
                else {
                    forward_status  = forward_solver.bitset_solve(forward_depth++, max_forward_depth);
                    printf("----------------------------------\n\n");
                    backward_status = backward_solver.bitset_solve(backward_depth++, max_backward_depth);
                    printf("-----------------------------------------------\n\n");
                }

                if (forward_status == 1 && backward_status == 1) {
                    solvable = true;
                    break;
                }

                bool found = this->find_intersection(forward_solver.visited(), backward_solver.visited());
                if (found) {
                    // Got a answer
                    solvable = true;
                    break;
                }

                if (iterative_type == 1) {
                    backward_solver.clear_prev_depth();
                }
                else if (iterative_type == 2) {
                    forward_solver.clear_prev_depth();
                }
                else {
                    forward_solver.clear_prev_depth();
                    backward_solver.clear_prev_depth();
                }
            }
            sw.stop();

            if (solvable) {
                double elapsed_time = sw.getElapsedMillisec();
                printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
                //System::pause();

                std::vector<Position> fw_move_path;
                Value128 fw_board_value = this->fw_answer_board_.value128();
                bool fw_found = forward_solver.find_board_in_last(fw_board_value, fw_move_path);
                if (fw_found) {
                    printf("Forward solver found value.\n\n");
                    printf("Move path size: %u\n\n", (std::uint32_t)fw_move_path.size());
                }

                std::vector<Position> bw_move_path;
                Value128 bw_board_value = this->bw_answer_board_.value128();
                bool bw_found = backward_solver.find_board_in_last(bw_board_value, bw_move_path);
                if (bw_found) {
                    printf("Backward solver found value.\n\n");
                    printf("Move path size: %u\n\n", (std::uint32_t)bw_move_path.size());
                }

                forward_solver.clear_prev_depth();
                backward_solver.clear_prev_depth();

                base_type::display_target_board("Target board:", forward_solver.getTargetBoard());
                base_type::display_player_board("Player board:", forward_solver.getPlayerBoard());

                base_type::display_player_board("Forward answer:", this->fw_answer_board_);
                base_type::display_player_board("Backward answer:", this->bw_answer_board_);

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
