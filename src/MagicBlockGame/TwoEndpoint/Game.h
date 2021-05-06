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

template <std::size_t BoardX, std::size_t BoardY>
struct SegmentPair {
    std::uint16_t fw_segments[BoardY];
    std::uint16_t bw_segments[BoardY];
};

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

    typedef SegmentPair<BoardX, BoardY> segment_pair_t;

private:
    Board<BoardX, BoardY> fw_answer_board_;
    Board<BoardX, BoardY> bw_answer_board_;

    segment_pair_t              segment_pair_;
    std::vector<segment_pair_t> segment_list_;

public:
    Game() : base_type() {
        this->init();
    }

    ~Game() {
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

    int travel_forward_visited_leaf(ForwardContainer * fw_container, BackwardContainer * bw_container, size_type layer) {
        assert(fw_container != nullptr);
        assert(bw_container != nullptr);
        int total = 0;
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
                    assert(layer < BoardY);
                    this->segment_pair_.fw_segments[layer] = fw_value;
                    this->segment_pair_.bw_segments[layer] = bw_value;

                    // Got a answer
                    this->segment_list_.push_back(this->segment_pair_);
                    total++;
                }
            }
        }
        return total;
    }

    int travel_forward_visited(ForwardContainer * fw_container, BackwardContainer * bw_container, size_type layer) {
        assert(fw_container != nullptr);
        assert(bw_container != nullptr);
        int total = 0;
        for (size_type i = fw_container->begin(); i < fw_container->end(); fw_container->next(i)) {
            int fw_value = fw_container->indexOf(i);
            assert(fw_value != -1);
            ForwardContainer * fw_child = fw_container->valueOf(i);
            assert (fw_child != nullptr);
            for (size_type j = bw_container->begin(); j < bw_container->end(); bw_container->next(j)) {
                int bw_value = bw_container->indexOf(j);
                assert(bw_value != -1);
                // It's overlapped ?
                bool overlapped = this->is_coincident(fw_value, bw_value);
                if (overlapped) {
                    // Record the board segment value of layer N
                    this->segment_pair_.fw_segments[layer] = fw_value;
                    this->segment_pair_.bw_segments[layer] = bw_value;
                    
                    BackwardContainer * bw_child = bw_container->valueOf(j);
                    assert (bw_child != nullptr);

                    if (!fw_child->isLeaf()) {
                        // Travel the next layer if it's not a leaf container
                        int count = this->travel_forward_visited(fw_child, bw_child, layer + 1);
                        total += count;
                    }
                    else {
                        // Search and compare the leaf containers
                        int count = this->travel_forward_visited_leaf(fw_child, bw_child, layer + 1);
                        total += count;
                    }
                }
            }
        }
        return total;
    }

    int find_intersection(typename TForwardSolver::bitset_type & forward_visited,
                          typename TBackwardSolver::bitset_type & backward_visited) {
        ForwardContainer * fw_container = forward_visited.root();
        if (fw_container == nullptr)
            return false;

        BackwardContainer * bw_container = backward_visited.root();
        if (bw_container == nullptr)
            return false;

        int total = 0;
        this->segment_list_.clear();

        for (size_type i = fw_container->begin(); i < fw_container->end(); fw_container->next(i)) {
            int fw_value = fw_container->indexOf(i);
            if (fw_value != -1) {
                ForwardContainer * fw_child = fw_container->valueOf(i);
                assert(fw_child != nullptr);
                for (size_type j = bw_container->begin(); j < bw_container->end(); bw_container->next(j)) {
                    int bw_value = bw_container->indexOf(j);
                    if (bw_value != -1) {
                        // It's overlapped ?
                        bool overlapped = this->is_coincident(fw_value, bw_value);
                        if (overlapped) {
                            // Record the board segment value of layer 0
                            this->segment_pair_.fw_segments[0] = fw_value;
                            this->segment_pair_.bw_segments[0] = bw_value;

                            BackwardContainer * bw_child = bw_container->valueOf(j);
                            assert(bw_child != nullptr);

                            // Travel the next layer
                            int count = this->travel_forward_visited(fw_child, bw_child, 1);
                            total += count;
                        }
                    }
                }
            }
        }

        return total;
    }

    size_type merge_move_path(std::vector<Position> & move_path,
                              TBackwardSolver & backward_solver,
                              const stage_type & fw_stage,
                              const stage_type & bw_stage) {
        // Copy the move path of forward stage first
        move_path = fw_stage.move_path;

        // Translate backward stage move path to move info
        if (backward_solver.translateMovePath(bw_stage)) {
            const std::vector<MoveInfo> & bw_answer = backward_solver.getAnswer();
            for (ssize_type i = bw_answer.size() - 1; i >= 0; i--) {
                MoveInfo move_info = bw_answer[i];
                // Opposite the from_pos and move_pos
                move_path.push_back(move_info.move_pos);
            }
        }

        return move_path.size();
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

                int total = this->find_intersection(forward_solver.visited(), backward_solver.visited());
                if (this->segment_list_.size() > 0) {
                    // Got some answers
                    assert(total == (int)this->segment_list_.size());
                    printf("Got some answers: %d\n\n", total);
                    solvable = true;
                    //System::pause();
#if 1
                    stage_type fw_stage;
                    stage_type bw_stage;

                    for (size_type i = 0; i < this->segment_list_.size(); i++) {
                        forward_solver.visited().compose_segments_to_board(this->fw_answer_board_, this->segment_list_[i].fw_segments);
                        backward_solver.visited().compose_segments_to_board(this->bw_answer_board_, this->segment_list_[i].bw_segments);

                        fw_stage.move_path.clear();

                        Value128 fw_board_value = this->fw_answer_board_.value128();
                        bool fw_found = forward_solver.find_stage_in_list(fw_board_value, fw_stage);
                        if (fw_found) {
                            printf("-----------------------------------------------\n\n");
                            printf("ForwardSolver: found target value in last depth.\n\n");
                            printf("Move path size: %u\n\n", (std::uint32_t)fw_stage.move_path.size());
                        }
                        else {
                            TForwardSolver forward_solver2(&this->data_);
                            forward_status = forward_solver2.bitset_find_stage(fw_board_value, fw_stage, max_forward_depth);
                            if (forward_status == 1) {
                                fw_found = true;
                                printf("-----------------------------------------------\n\n");
                                printf("ForwardSolver::bitset_find_stage(): found target value.\n\n");
                                printf("Move path size: %u\n\n", (std::uint32_t)fw_stage.move_path.size());
                            }
                        }

                        bw_stage.move_path.clear();

                        Value128 bw_board_value = this->bw_answer_board_.value128();
                        bool bw_found = backward_solver.find_stage_in_list(bw_board_value, bw_stage);
                        if (bw_found) {
                            printf("-----------------------------------------------\n\n");
                            printf("BackwardSolver: found target value in last depth.\n\n");
                            printf("Move path size: %u\n\n", (std::uint32_t)bw_stage.move_path.size());
                        }
                        else {
                            TBackwardSolver backward_solver2(&this->data_);
                            backward_status = backward_solver2.bitset_find_stage(bw_board_value, bw_stage, max_backward_depth);
                            if (backward_status == 1) {
                                bw_found = true;
                                printf("-----------------------------------------------\n\n");
                                printf("BackwardSolver::bitset_find_stage(): found target value.\n\n");
                                printf("Move path size: %u\n\n", (std::uint32_t)bw_stage.move_path.size());
                            }
                        }

                        if (fw_found && bw_found) {
                            std::vector<Position> & fw_move_path = fw_stage.move_path;
                            std::vector<Position> & bw_move_path = bw_stage.move_path;

                            size_type total_steps = this->merge_move_path(this->move_path_, backward_solver, fw_stage, bw_stage);
                            size_type total_size = fw_move_path.size() + bw_move_path.size();
                            (void)total_size;
                            assert(total_steps == total_size);
                            if (total_steps < this->min_steps_) {
                                base_type::display_target_board("Target board:", forward_solver.getTargetBoard());
                                base_type::display_player_board("Player board:", forward_solver.getPlayerBoard());

                                base_type::display_player_board("Forward answer:", this->fw_answer_board_);
                                base_type::display_player_board("Backward answer:", this->bw_answer_board_);

                                if (this->translateMovePath(fw_move_path)) {
                                    this->displayAnswer();
                                }

                                size_type n_rotate_type = bw_stage.rotate_type;
                                size_type empty_pos = n_rotate_type >> 2;
                                size_type rotate_type = n_rotate_type & 0x03;
                                printf("backward_solver: rotate_type = %u, empty_pos = %u\n\n",
                                       (uint32_t)rotate_type, (uint32_t)empty_pos);

                                if (backward_solver.translateMovePath(bw_stage)) {
                                    backward_solver.displayAnswer();
                                }

                                printf("-----------------------------------------------\n\n");
                                printf("Forward moves: %u, Backward moves: %u, Total moves: %u\n\n",
                                        (uint32_t)fw_move_path.size(),
                                        (uint32_t)bw_move_path.size(),
                                        (uint32_t)total_steps);
                                this->map_used_ = forward_solver.getMapUsed() + backward_solver.getMapUsed();
                                this->min_steps_ = total_steps;
                                this->best_move_path_ = this->move_path_;
                                printf("Total moves: %u\n\n", (uint32_t)this->best_move_path_.size());

                                if (this->translateMovePath(this->best_move_path_)) {
                                    this->displayAnswer();
                                }
                                //System::pause();
                            }
                        }
                    }

                    if (this->segment_list_.size() > 1) {
                        forward_solver.clear_prev_depth();
                        backward_solver.clear_prev_depth();
                        break;
                    }
#endif
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
                if (this->translateMovePath(this->best_move_path_)) {
                    this->displayAnswer();
                }
                //System::pause();
#if 0
                stage_type fw_stage;
                stage_type bw_stage;

                for (size_type i = 0; i < this->segment_list_.size(); i++) {
                    forward_solver.visited().compose_segments_to_board(this->fw_answer_board_, this->segment_list_[i].fw_segments);
                    backward_solver.visited().compose_segments_to_board(this->bw_answer_board_, this->segment_list_[i].bw_segments);

                    fw_stage.move_path.clear();
                    Value128 fw_board_value = this->fw_answer_board_.value128();
                    bool fw_found = forward_solver.find_stage_in_list(fw_board_value, fw_stage);
                    if (fw_found) {
                        printf("-----------------------------------------------\n\n");
                        printf("ForwardSolver: found target value in last depth.\n\n");
                        printf("Move path size: %u\n\n", (std::uint32_t)fw_stage.move_path.size());
                    }

                    bw_stage.move_path.clear();
                    Value128 bw_board_value = this->bw_answer_board_.value128();
                    bool bw_found = backward_solver.find_stage_in_list(bw_board_value, bw_stage);
                    if (bw_found) {
                        printf("-----------------------------------------------\n\n");
                        printf("BackwardSolver: found target value in last depth.\n\n");
                        printf("Move path size: %u\n\n", (std::uint32_t)bw_stage.move_path.size());
                    }

                    if (fw_found && bw_found) {
                        std::vector<Position> & fw_move_path = fw_stage.move_path;
                        std::vector<Position> & bw_move_path = bw_stage.move_path;

                        size_type total_steps = fw_move_path.size() + bw_move_path.size();
                        printf("-----------------------------------------------\n\n");
                        printf("Forward moves: %u, Backward moves: %u, Total moves: %u\n\n",
                                (uint32_t)fw_move_path.size(),
                                (uint32_t)bw_move_path.size(),
                                (uint32_t)total_steps);

                        if (total_steps < this->min_steps_) {
                            this->map_used_ = forward_solver.getMapUsed();
                            this->min_steps_ = total_steps;
                            this->best_move_path_ = fw_move_path;
                            for (ssize_type i = bw_move_path.size() - 1; i >= 0; i--) {
                                this->best_move_path_.push_back(bw_move_path[i]);
                            }
                            printf("Total moves: %u\n\n", (uint32_t)this->best_move_path_.size());
                        }
                    }
                }

                printf("-----------------------------------------------\n\n");
#endif

#if 0
                stage_type fw_stage;
                stage_type bw_stage;

                forward_solver.visited().compose_segments_to_board(this->fw_answer_board_, this->segment_pair_.fw_segments);
                backward_solver.visited().compose_segments_to_board(this->bw_answer_board_, this->segment_pair_.bw_segments);

                fw_stage.move_path.clear();
                Value128 fw_board_value = this->fw_answer_board_.value128();
                bool fw_found = forward_solver.find_stage_in_list(fw_board_value, fw_stage);
                if (fw_found) {
                    printf("-----------------------------------------------\n\n");
                    printf("ForwardSolver: found target value in last depth.\n\n");
                    printf("Move path size: %u\n\n", (std::uint32_t)fw_stage.move_path.size());
                }

                bw_stage.move_path.clear();
                Value128 bw_board_value = this->bw_answer_board_.value128();
                bool bw_found = backward_solver.find_stage_in_list(bw_board_value, bw_stage);
                if (bw_found) {
                    printf("-----------------------------------------------\n\n");
                    printf("BackwardSolver: found target value in last depth.\n\n");
                    printf("Move path size: %u\n\n", (std::uint32_t)bw_stage.move_path.size());
                }

                forward_solver.clear_prev_depth();
                backward_solver.clear_prev_depth();

                base_type::display_target_board("Target board:", forward_solver.getTargetBoard());
                base_type::display_player_board("Player board:", forward_solver.getPlayerBoard());

                base_type::display_player_board("Forward answer:", this->fw_answer_board_);
                base_type::display_player_board("Backward answer:", this->bw_answer_board_);

                printf("-----------------------------------------------\n\n");
                System::pause();

                fw_stage.move_path.clear();
                forward_solver.respawn();

                forward_status = forward_solver.bitset_find_stage(fw_board_value, fw_stage, max_forward_depth);
                if (forward_status == 1) {
                    printf("-----------------------------------------------\n\n");
                    printf("ForwardSolver::bitset_find_stage(): found target value.\n\n");
                    printf("Move path size: %u\n\n", (std::uint32_t)fw_stage.move_path.size());
                }

                printf("-----------------------------------------------\n\n");

                bw_stage.move_path.clear();
                backward_solver.respawn();

                backward_status = backward_solver.bitset_find_stage(bw_board_value, bw_stage, max_backward_depth);
                if (backward_status == 1) {
                    printf("-----------------------------------------------\n\n");
                    printf("BackwardSolver::bitset_find_stage(): found target value.\n\n");
                    printf("Move path size: %u\n\n", (std::uint32_t)bw_stage.move_path.size());
                }

                printf("-----------------------------------------------\n\n");
#endif

#if 0
                this->move_path_ = backward_solver.getMovePath();
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
                    for (ssize_type i = this->move_path_.size() - 1; i >= 0; i--) {
                        this->best_move_path_.push_back(this->move_path_[i]);
                    }
                    printf("Total moves: %u\n\n", (uint32_t)this->best_move_path_.size());
                }

                printf("min_steps: %u\n", (uint32_t)this->min_steps_);
                printf("Total moves: %u\n", (uint32_t)this->best_move_path_.size());
                printf("\n");

                if (this->min_steps_ != size_type(-1) || this->best_move_path_.size() > 0) {
                    solvable = true;
                    if (this->translateMovePath(forward_solver.getMovePath())) {
                        this->displayAnswer();
                    }

                    size_type n_rotate_type = backward_solver.getRotateType();
                    size_type empty_pos = n_rotate_type >> 2;
                    size_type rotate_type = n_rotate_type & 0x03;
                    printf("backward_solver: rotate_type = %u, empty_pos = %u\n\n",
                           (uint32_t)rotate_type, (uint32_t)empty_pos);

                    if (backward_solver.translateMovePath(backward_solver.getMovePath(), rotate_type, empty_pos)) {
                        backward_solver.displayAnswer();
                    }
                }
#endif
            }
        }

        return solvable;
    }
};

} // namespace TwoEndpoint
} // namespace MagicBlock
