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
#include "MagicBlock/AI/TwoEndpoint/ForwardSolver.h"
#include "MagicBlock/AI/TwoEndpoint/BackwardSolver.h"

#include "MagicBlock/AI/Constant.h"
#include "MagicBlock/AI/Color.h"
#include "MagicBlock/AI/Move.h"
#include "MagicBlock/AI/MoveSeq.h"
#include "MagicBlock/AI/CanMoves.h"
#include "MagicBlock/AI/Board.h"
#include "MagicBlock/AI/SharedData.h"
#include "MagicBlock/AI/ErrorCode.h"
#include "MagicBlock/AI/Console.h"
#include "MagicBlock/AI/StopWatch.h"
#include "MagicBlock/AI/Utils.h"

//
// Two endpoint / two leg algorithm
//
namespace MagicBlock {
namespace AI {
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

    std::vector<std::pair<Value128, Value128>> board_value_list_;

public:
    Game() : base_type() {
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
        if (std::uint32_t(Color::Unknown) == 0x00000007U) {
            if ((bw_value32 & 0x00000007U) == std::uint32_t(Color::Unknown)) {
                if ((fw_value32 & 0x00000007U) != std::uint32_t(Color::Empty)) {
                    fw_value32 |= 0x00000007U;
                }
            }
            if ((bw_value32 & 0x00000038U) == std::uint32_t(Color::Unknown << 3U)) {
                if ((fw_value32 & 0x00000038U) != std::uint32_t(Color::Empty << 3U)) {
                    fw_value32 |= 0x00000038U;
                }
            }
            if ((bw_value32 & 0x000001C0U) == std::uint32_t(Color::Unknown << 6U)) {
                if ((fw_value32 & 0x000001C0U) != std::uint32_t(Color::Empty << 6U)) {
                    fw_value32 |= 0x000001C0U;
                }
            }
            if ((bw_value32 & 0x00000E00U) == std::uint32_t(Color::Unknown << 9U)) {
                if ((fw_value32 & 0x00000E00U) != std::uint32_t(Color::Empty << 9U)) {
                    fw_value32 |= 0x00000E00U;
                }
            }
            if ((bw_value32 & 0x00007000U) == std::uint32_t(Color::Unknown << 12U)) {
                if ((fw_value32 & 0x00007000U) != std::uint32_t(Color::Empty << 12U)) {
                    fw_value32 |= 0x00007000U;
                }
            }
        }
        else {
            if ((bw_value32 & 0x00000007U) == std::uint32_t(Color::Unknown)) {
                if ((fw_value32 & 0x00000007U) != std::uint32_t(Color::Empty)) {
                    fw_value32 ^= 0x00000007U;
                    fw_value32 |= std::uint32_t(Color::Unknown);
                }
            }
            if ((bw_value32 & 0x00000038U) == std::uint32_t(Color::Unknown << 3U)) {
                if ((fw_value32 & 0x00000038U) != std::uint32_t(Color::Empty << 3U)) {
                    fw_value32 ^= 0x00000038U;
                    fw_value32 |= std::uint32_t(Color::Unknown << 3U);
                }
            }
            if ((bw_value32 & 0x000001C0U) == std::uint32_t(Color::Unknown << 6U)) {
                if ((fw_value32 & 0x000001C0U) != std::uint32_t(Color::Empty << 6U)) {
                    fw_value32 ^= 0x000001C0U;
                    fw_value32 |= std::uint32_t(Color::Unknown << 6U);
                }
            }
            if ((bw_value32 & 0x00000E00U) == std::uint32_t(Color::Unknown << 9U)) {
                if ((fw_value32 & 0x00000E00U) != std::uint32_t(Color::Empty << 9U)) {
                    fw_value32 ^= 0x00000E00U;
                    fw_value32 |= std::uint32_t(Color::Unknown << 9U);
                }
            }
            if ((bw_value32 & 0x00007000U) == std::uint32_t(Color::Unknown << 12U)) {
                if ((fw_value32 & 0x00007000U) != std::uint32_t(Color::Empty << 12U)) {
                    fw_value32 ^= 0x00007000U;
                    fw_value32 |= std::uint32_t(Color::Unknown << 12U);
                }
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

    bool is_coincident(Value128 fw_value, Value128 bw_value) const {
        static const size_type colorMask = 0x07;
        static const size_type colorShift = 3;

        size_type fw_color, bw_color;
        if (BoardSize <= 21) {
            // Low bit 0~62
            for (ssize_type pos = BoardSize - 1; pos >= 0; pos--) {
                fw_color = fw_value.low & colorMask;
                bw_color = bw_value.low & colorMask;
                if (bw_color == Color::Unknown) {
                    if (fw_color == Color::Empty) {
                        return false;
                    }
                }
                else if (fw_color != bw_color) {
                    return false;
                }
                fw_value.low >>= colorShift;
                bw_value.low >>= colorShift;
            }
        }
        else {
            // Low bit 0~62
            for (ssize_type pos = 20; pos >= 0; pos--) {
                fw_color = fw_value.low & colorMask;
                bw_color = bw_value.low & colorMask;
                if (bw_color == Color::Unknown) {
                    if (fw_color == Color::Empty)
                        return false;
                }
                else if (fw_color != bw_color) {
                    return false;
                }
                fw_value.low >>= colorShift;
                bw_value.low >>= colorShift;
            }

            // Low bit 63 and High bit 0~1
            fw_color = fw_value.low | ((fw_value.high << 1) | colorMask);
            bw_color = bw_value.low | ((bw_value.high << 1) | colorMask);

            fw_value.high >>= (colorShift - 1);
            bw_value.high >>= (colorShift - 1);

            // High bit 2~63
            for (ssize_type pos = BoardSize - 1; pos >= 21; pos--) {
                fw_color = fw_value.high & colorMask;
                bw_color = bw_value.high & colorMask;
                fw_value.high >>= colorShift;
                bw_value.high >>= colorShift;
                if (bw_color == Color::Unknown) {
                    if (fw_color == Color::Empty)
                        return false;
                }
                else if (fw_color != bw_color) {
                    return false;
                }
            }
        }
        return true;
    }

    template <size_type First, size_type Last>
    bool is_coincident_low(std::uint64_t fw_value, std::uint64_t bw_value) const {
        static const size_type colorMask = 0x07;
        static const size_type colorShift = 3;
        static const size_type first = First;
        static const size_type last = (Last < 21) ? Last : 21;

        size_type fw_color, bw_color;
        fw_value >>= (first * colorShift);
        bw_value >>= (first * colorShift);
        // Low bit 0~62
        for (size_type pos = first; pos < last; pos++) {
            fw_color = fw_value & colorMask;
            bw_color = bw_value & colorMask;
            if (bw_color == Color::Unknown) {
                if (fw_color == Color::Empty) {
                    return false;
                }
            }
            else if (fw_color != bw_color) {
                return false;
            }
            fw_value >>= colorShift;
            bw_value >>= colorShift;
        }
        return true;
    }

    template <size_type First, size_type Last>
    bool is_coincident(Value128 fw_value, Value128 bw_value) const {
        static const size_type colorMask = 0x07;
        static const size_type colorShift = 3;
        static const size_type first = First;
        static const size_type last = (Last < BoardSize) ? Last : BoardSize;

        size_type fw_color, bw_color;
        if (BoardSize <= 21) {
            fw_value.low >>= (first * colorShift);
            bw_value.low >>= (first * colorShift);
            // Low bit 0~62
            for (size_type pos = first; pos < last; pos++) {
                fw_color = fw_value.low & colorMask;
                bw_color = bw_value.low & colorMask;
                if (bw_color == Color::Unknown) {
                    if (fw_color == Color::Empty) {
                        return false;
                    }
                }
                else if (fw_color != bw_color) {
                    return false;
                }
                fw_value.low >>= colorShift;
                bw_value.low >>= colorShift;
            }
        }
        else {
            if (Last <= 20) {
                fw_value.low >>= (first * colorShift);
                bw_value.low >>= (first * colorShift);
                // Low bit 0~62
                for (size_type pos = first; pos < last; pos++) {
                    fw_color = fw_value.low & colorMask;
                    bw_color = bw_value.low & colorMask;
                    if (bw_color == Color::Unknown) {
                        if (fw_color == Color::Empty)
                            return false;
                    }
                    else if (fw_color != bw_color) {
                        return false;
                    }
                    fw_value.low >>= colorShift;
                    bw_value.low >>= colorShift;
                }
            }
            else {
                if ((First <= 21 && Last >= 21)) {
                    fw_value.low >>= (first * colorShift);
                    bw_value.low >>= (first * colorShift);
                    // Low bit 0~62
                    for (size_type pos = first; pos < 21; pos++) {
                        fw_color = fw_value.low & colorMask;
                        bw_color = bw_value.low & colorMask;
                        if (bw_color == Color::Unknown) {
                            if (fw_color == Color::Empty)
                                return false;
                        }
                        else if (fw_color != bw_color) {
                            return false;
                        }
                        fw_value.low >>= colorShift;
                        bw_value.low >>= colorShift;
                    }

                    // Low bit 63 and High bit 0~1
                    fw_color = (fw_value.low & colorMask) | ((fw_value.high << 1) & colorMask);
                    bw_color = (bw_value.low & colorMask) | ((bw_value.high << 1) & colorMask);

                    if (bw_color == Color::Unknown) {
                        if (fw_color == Color::Empty)
                            return false;
                    }
                    else if (fw_color != bw_color) {
                        return false;
                    }
                }

                if (Last > 21) {
                    static const size_type kHighSkipUnits = (First <= 21) ? 0 : (First - 21);
                    fw_value.high >>= (colorShift - 1) + kHighSkipUnits * colorShift;
                    bw_value.high >>= (colorShift - 1) + kHighSkipUnits * colorShift;

                    // High bit 2~63
                    size_type pos = ((First <= 21 && Last >= 21)) ? (first + 2) : first;
                    for (; pos < last; pos++) {
                        fw_color = fw_value.high & colorMask;
                        bw_color = bw_value.high & colorMask;
                        if (bw_color == Color::Unknown) {
                            if (fw_color == Color::Empty)
                                return false;
                        }
                        else if (fw_color != bw_color) {
                            return false;
                        }
                        fw_value.high >>= colorShift;
                        bw_value.high >>= colorShift;
                    }
                }
            }
        }
        return true;
    }

    int find_intersection(typename TForwardSolver::stdset_type & forward_visited,
                          typename TBackwardSolver::stdset_type & backward_visited) {
        this->board_value_list_.clear();

        std::vector<std::pair<Value128, Value128>> curr_set;
        std::vector<std::pair<Value128, Value128>> next_set;

        for (auto const & fw_value128 : forward_visited) {
            for (auto const & bw_value128 : backward_visited) {
                if (this->template is_coincident_low<10, 15>(fw_value128.low, bw_value128.low)) {
                    curr_set.push_back(std::make_pair(fw_value128, bw_value128));
                }
            }
        }

        if (curr_set.size() == 0)
            return 0;

        for (auto const & val_pair : curr_set) {
            if (this->template is_coincident_low<5, 10>(val_pair.first.low, val_pair.second.low)) {
                next_set.push_back(val_pair);
            }
        }

        if (next_set.size() == 0)
            return 0;

        std::swap(curr_set, next_set);
        next_set.clear();
        next_set.shrink_to_fit();

        for (auto const & val_pair : curr_set) {
            if (this->template is_coincident_low<15, 20>(val_pair.first.low, val_pair.second.low)) {
                next_set.push_back(val_pair);
            }
        }

        if (next_set.size() == 0)
            return 0;

        std::swap(curr_set, next_set);
        next_set.clear();
        next_set.shrink_to_fit();

        for (auto const & val_pair : curr_set) {
            if (this->template is_coincident_low<0, 5>(val_pair.first.low, val_pair.second.low)) {
                next_set.push_back(val_pair);
            }
        }

        if (next_set.size() == 0)
            return 0;

        std::swap(curr_set, next_set);
        next_set.clear();

        int total = 0;
        for (auto const & val_pair : curr_set) {
            if (this->template is_coincident<20, 25>(val_pair.first, val_pair.second)) {
                this->board_value_list_.push_back(val_pair);
                total++;
            }
        }
        return total;
    }

    size_type merge_move_seq(MoveSeq & move_seq,
                             TBackwardSolver & backward_solver,
                             const stage_type & fw_stage,
                             const stage_type & bw_stage) {
        // Copy the move path of forward stage first
        move_seq = fw_stage.move_seq;
#if 1
        // Translate backward stage move path to move info
        const MoveSeq & bw_move_seq = bw_stage.move_seq;
        for (ssize_type i = bw_move_seq.size() - 1; i >= 0; i--) {
            std::uint8_t move_dir = (std::uint8_t)bw_move_seq[i];
            std::uint8_t opp_dir = Dir::getOppDir(move_dir);
            // Opposite the move dir
            move_seq.push_back(opp_dir);
        }
#else
        // Translate backward stage move path to move info
        if (backward_solver.translateMoveSeq(bw_stage)) {
            const std::vector<MoveInfo> & bw_answer = backward_solver.getAnswer();
            for (ssize_type i = bw_answer.size() - 1; i >= 0; i--) {
                MoveInfo move_info = bw_answer[i];
                // Opposite the from_pos and move_pos
                move_seq.push_back(move_info.to_pos);
            }
        }
#endif
        return move_seq.size();
    }

    bool stdset_solve(size_type max_forward_depth, size_type max_backward_depth) {
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
            while (forward_depth < max_forward_depth || backward_depth < max_backward_depth) {
                int iterative_type = 0;
#if 1
                if (forward_depth > 15) {
                    size_type fw_visited_size = forward_solver.visited().size();
                    size_type bw_visited_size = backward_solver.visited().size();
                    if (forward_depth >= max_forward_depth || fw_visited_size >= bw_visited_size * 2) {
                        iterative_type = 1;
                    }
                    else if (backward_depth >= max_backward_depth || bw_visited_size >= fw_visited_size * 2) {
                        iterative_type = 2;
                    }
                }
#endif
                if (iterative_type == 1) {
                    forward_status  = 0;
                    backward_status = backward_solver.stdset_solve(backward_depth++, max_backward_depth);
                    printf("-----------------------------------------------\n\n");
                }
                else if (iterative_type == 2) {
                    forward_status  = forward_solver.stdset_solve(forward_depth++, max_forward_depth);
                    backward_status = 0;
                    printf("-----------------------------------------------\n\n");
                }
                else {
                    forward_status  = forward_solver.stdset_solve(forward_depth++, max_forward_depth);
                    printf("----------------------------------\n\n");
                    backward_status = backward_solver.stdset_solve(backward_depth++, max_backward_depth);
                    printf("-----------------------------------------------\n\n");
                }

                (void)forward_status;
                (void)backward_status;

                int total = this->find_intersection(forward_solver.visited_set(), backward_solver.visited_set());
                if (this->board_value_list_.size() > 0) {
                    // Got some answers
                    assert(total == (int)this->board_value_list_.size());
                    printf("Got some answers: %d\n\n", total);

                    stage_type fw_stage;
                    stage_type bw_stage;

                    for (size_type i = 0; i < this->board_value_list_.size(); i++) {
                        fw_stage.move_seq.clear();

                        Value128 fw_board_value = this->board_value_list_[i].first;
                        bool fw_found = forward_solver.find_stage_in_list(fw_board_value, fw_stage);
                        if (fw_found) {
                            printf("-----------------------------------------------\n\n");
                            printf("ForwardSolver: found target value in last depth.\n\n");
                            printf("Move path size: %u\n\n", (std::uint32_t)fw_stage.move_seq.size());
                        }

                        bw_stage.move_seq.clear();

                        Value128 bw_board_value = this->board_value_list_[i].second;
                        bool bw_found = backward_solver.find_stage_in_list(bw_board_value, bw_stage);
                        if (bw_found) {
                            printf("-----------------------------------------------\n\n");
                            printf("BackwardSolver: found target value in last depth.\n\n");
                            printf("Move path size: %u\n\n", (std::uint32_t)bw_stage.move_seq.size());
                        }

                        if (fw_found && bw_found) {
                            MoveSeq & fw_move_seq = fw_stage.move_seq;
                            MoveSeq & bw_move_seq = bw_stage.move_seq;

                            size_type total_steps = this->merge_move_seq(this->move_seq_, backward_solver, fw_stage, bw_stage);
                            size_type total_size = fw_move_seq.size() + bw_move_seq.size();
                            (void)total_size;
                            assert(total_steps == total_size);
                            if (total_steps < this->min_steps_) {
                                solvable = true;

                                Board<BoardX, BoardY>::display_board("Player board:", forward_solver.getPlayerBoard());
                                Board<TargetX, TargetY>::display_board("Target board:", forward_solver.getTargetBoard());

                                Board<BoardX, BoardY>::display_board("Forward answer:", this->fw_answer_board_);
                                Board<BoardX, BoardY>::display_board("Backward answer:", this->bw_answer_board_);

                                this->displayMoveList(fw_stage);

                                size_type n_rotate_type = bw_stage.rotate_type;
                                size_type empty_pos = n_rotate_type >> 2;
                                size_type rotate_type = n_rotate_type & 0x03;
                                printf("backward_solver: rotate_type = %u, empty_pos = %u\n\n",
                                       (uint32_t)rotate_type, (uint32_t)empty_pos);

                                backward_solver.displayMoveList(bw_stage);

                                printf("-----------------------------------------------\n\n");
                                printf("Forward moves: %u, Backward moves: %u, Total moves: %u\n\n",
                                        (uint32_t)fw_move_seq.size(),
                                        (uint32_t)bw_move_seq.size(),
                                        (uint32_t)total_steps);
                                this->map_used_ = forward_solver.getMapUsed() + backward_solver.getMapUsed();
                                this->min_steps_ = total_steps;
                                this->best_move_seq_ = this->move_seq_;
                                printf("Total moves: %u\n\n", (uint32_t)this->best_move_seq_.size());

                                this->displayMoveList();
                                //Console::readKeyLine();
                            }
                        }
                    }

                    if (this->board_value_list_.size() > 0) {
                        forward_solver.clear_prev_depth();
                        backward_solver.clear_prev_depth();
                        break;
                    }
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

                this->displayMoveList();
            }
        }

        return solvable;
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
            while (forward_depth < max_forward_depth || backward_depth < max_backward_depth) {
                int iterative_type = 0;
#if 1
                if (forward_depth > 15) {
                    size_type fw_visited_size = forward_solver.visited().size();
                    size_type bw_visited_size = backward_solver.visited().size();
                    if (forward_depth >= max_forward_depth || fw_visited_size >= bw_visited_size * 2) {
                        iterative_type = 1;
                    }
                    else if (backward_depth >= max_backward_depth || bw_visited_size >= fw_visited_size * 2) {
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

                (void)forward_status;
                (void)backward_status;

                int total = this->find_intersection(forward_solver.visited(), backward_solver.visited());
                if (this->segment_list_.size() > 0) {
                    // Got some answers
                    assert(total == (int)this->segment_list_.size());
                    printf("Got some answers: %d\n\n", total);

                    stage_type fw_stage;
                    stage_type bw_stage;

                    for (size_type i = 0; i < this->segment_list_.size(); i++) {
                        forward_solver.visited().compose_segments_to_board(this->fw_answer_board_, this->segment_list_[i].fw_segments);
                        backward_solver.visited().compose_segments_to_board(this->bw_answer_board_, this->segment_list_[i].bw_segments);

                        fw_stage.move_seq.clear();

                        Value128 fw_board_value = this->fw_answer_board_.value128();
                        bool fw_found = forward_solver.find_stage_in_list(fw_board_value, fw_stage);
                        if (fw_found) {
                            printf("-----------------------------------------------\n\n");
                            printf("ForwardSolver: found target value in last depth.\n\n");
                            printf("Move path size: %u\n\n", (std::uint32_t)fw_stage.move_seq.size());
                        }
                        else {
                            TForwardSolver forward_solver2(&this->data_);
                            forward_status = forward_solver2.bitset_find_stage(fw_board_value, fw_stage, max_forward_depth);
                            if (forward_status == 1) {
                                fw_found = true;
                                printf("-----------------------------------------------\n\n");
                                printf("ForwardSolver::bitset_find_stage(): found target value.\n\n");
                                printf("Move path size: %u\n\n", (std::uint32_t)fw_stage.move_seq.size());
                            }
                        }

                        bw_stage.move_seq.clear();

                        Value128 bw_board_value = this->bw_answer_board_.value128();
                        bool bw_found = backward_solver.find_stage_in_list(bw_board_value, bw_stage);
                        if (bw_found) {
                            printf("-----------------------------------------------\n\n");
                            printf("BackwardSolver: found target value in last depth.\n\n");
                            printf("Move path size: %u\n\n", (std::uint32_t)bw_stage.move_seq.size());
                        }
                        else {
                            TBackwardSolver backward_solver2(&this->data_);
                            backward_status = backward_solver2.bitset_find_stage(bw_board_value, bw_stage, max_backward_depth);
                            if (backward_status == 1) {
                                bw_found = true;
                                printf("-----------------------------------------------\n\n");
                                printf("BackwardSolver::bitset_find_stage(): found target value.\n\n");
                                printf("Move path size: %u\n\n", (std::uint32_t)bw_stage.move_seq.size());
                            }
                        }

                        if (fw_found && bw_found) {
                            MoveSeq & fw_move_seq = fw_stage.move_seq;
                            MoveSeq & bw_move_seq = bw_stage.move_seq;

                            size_type total_steps = this->merge_move_seq(this->move_seq_, backward_solver, fw_stage, bw_stage);
                            size_type total_size = fw_move_seq.size() + bw_move_seq.size();
                            (void)total_size;
                            assert(total_steps == total_size);
                            if (total_steps < this->min_steps_) {
                                solvable = true;

                                Board<BoardX, BoardY>::display_board("Player board:", forward_solver.getPlayerBoard());
                                Board<TargetX, TargetY>::display_board("Target board:", forward_solver.getTargetBoard());

                                Board<BoardX, BoardY>::display_board("Forward answer:", this->fw_answer_board_);
                                Board<BoardX, BoardY>::display_board("Backward answer:", this->bw_answer_board_);

                                this->displayMoveList(fw_stage);

                                size_type n_rotate_type = bw_stage.rotate_type;
                                size_type empty_pos = n_rotate_type >> 2;
                                size_type rotate_type = n_rotate_type & 0x03;
                                printf("backward_solver: rotate_type = %u, empty_pos = %u\n\n",
                                       (uint32_t)rotate_type, (uint32_t)empty_pos);

                                backward_solver.displayMoveList(bw_stage);

                                printf("-----------------------------------------------\n\n");
                                printf("Forward moves: %u, Backward moves: %u, Total moves: %u\n\n",
                                        (uint32_t)fw_move_seq.size(),
                                        (uint32_t)bw_move_seq.size(),
                                        (uint32_t)total_steps);
                                this->map_used_ = forward_solver.getMapUsed() + backward_solver.getMapUsed();
                                this->min_steps_ = total_steps;
                                this->best_move_seq_ = this->move_seq_;
                                printf("Total moves: %u\n\n", (uint32_t)this->best_move_seq_.size());

                                this->displayMoveList();
                                //Console::readKeyLine();
                            }
                        }
                    }

                    if (this->segment_list_.size() > 0) {
                        forward_solver.clear_prev_depth();
                        backward_solver.clear_prev_depth();
                        break;
                    }
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

                this->displayMoveList();
            }
        }

        return solvable;
    }
};

} // namespace TwoEndpoint
} // namespace AI
} // namespace MagicBlock
