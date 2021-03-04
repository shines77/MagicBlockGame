#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <set>
#include <exception>
#include <stdexcept>
#include <algorithm>    // For std::swap(), until C++11. std::min()
#include <utility>      // For std::swap(), since C++11

#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "Stage.h"
#include "SharedData.h"
#include "SlidingPuzzle.h"

//#define ENABLE_DEBUG

namespace PuzzleGame {

template <size_t BoardX, size_t BoardY,
          size_t TargetX, size_t TargetY,
          size_t Step>
class MagicBlockSolver
{
public:
    static const size_t kSingelColorNums = (BoardX * BoardY - 1) / (Color::Last - 1);

    static const ptrdiff_t startX = (BoardX - TargetX) / 2;
    static const ptrdiff_t startY = (BoardY - TargetY) / 2;

    typedef typename SharedData<BoardX, BoardY, TargetX, TargetY>::stage_type stage_type;

private:
    SharedData<BoardX, BoardY, TargetX, TargetY> * data_;

    Board<BoardX, BoardY> board_;
    Board<TargetX, TargetY> target_;

    int board_colors_[Color::Maximum];
    int partial_colors_[Color::Maximum];
    int target_colors_[Color::Maximum];

    std::vector<stage_type> cur_;
    std::vector<stage_type> next_;

    std::set<uint128_t> visited_;
    std::vector<Move> moves_;

    size_t map_used_;

    void count_target_color_nums(const Board<TargetX, TargetY> & target) {
        for (size_t clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->data_->target_colors[clr] = 0;
        }

        for (size_t y = 0; y < TargetY; y++) {
            for (size_t x = 0; x < TargetX; x++) {
                char cell = target.cells[y * TargetY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->data_->target_colors[cell]++;
                }
            }
        }
    }

    void count_partial_target_color_nums(const Board<TargetX, TargetY> & target,
                                         size_t firstTargetX, size_t lastTargetX,
                                         size_t firstTargetY, size_t lastTargetY) {
        for (size_t clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->data_->target_colors[clr] = 0;
        }

        for (size_t y = firstTargetY; y < lastTargetY; y++) {
            for (size_t x = firstTargetX; x < lastTargetX; x++) {
                char cell = target.cells[y * TargetY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->data_->target_colors[cell]++;
                }
            }
        }
    }

    void locked_partial_board(int locked[BoardX * BoardY],
                              size_t firstX, size_t lastX,
                              size_t firstY, size_t lastY) {
        for (size_t y = 0; y < BoardY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_t x = 0; x < BoardX; x++) {
                locked[baseY + x] = 0;
            }
        }

        for (size_t y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_t x = firstX; x < lastX; x++) {
                locked[baseY + x] = 1;
            }
        }
    }

    void init() {
        if (Step == 1 || Step == 12 || Step == 123) {
            this->board_ = this->data_->board;
            this->target_ = this->data_->target;

            count_target_color_nums(this->target_);

            for (size_t i = 0; i < 4; i++) {
                this->data_->s123.min_depth[i] = -1;
                this->data_->s123.max_depth[i] = -1;
            }

            this->data_->s123.depth_limit = -1;

            for (size_t i = 0; i < 4; i++) {
                this->data_->s456.lock_inited[i] = 0;
            }
        }
        else if (Step == 456) {
            this->target_ = this->data_->target;
            size_t openning_type = this->data_->s456.openning_type;
            if (this->data_->s456.lock_inited[openning_type] == 0) {
                this->data_->s456.lock_inited[openning_type] = 1;
                switch (openning_type) {
                    case 0:
                        // Top partial
                        count_partial_target_color_nums(this->target_, 0, TargetX, TargetY - 1, TargetY);
                        locked_partial_board(this->data_->s456.locked, 0, BoardX, 0, startY + 1);
                        break;
                    case 1:
                        // Left partial
                        count_partial_target_color_nums(this->target_, TargetX - 1, TargetX, 0, TargetY);
                        locked_partial_board(this->data_->s456.locked, 0, startX + 1, 0, BoardY);
                        break;
                    case 2:
                        // Right partial
                        count_partial_target_color_nums(this->target_, 0, 1, 0, TargetY);
                        locked_partial_board(this->data_->s456.locked, startX + TargetX - 1, BoardX, 0, BoardY);
                        break;
                    case 3:
                        // Bottom partial
                        count_partial_target_color_nums(this->target_, 0, TargetX, 0, 1);
                        locked_partial_board(this->data_->s456.locked, 0, BoardX, startY + TargetY - 1, BoardY);
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
        }

        //count_board_color_nums(this->board_);
    }

    void assert_color(uint8_t color) const {
        assert(color >= Color::Empty && color <= Color::Last);
    }

public:
    MagicBlockSolver(SharedData<BoardX, BoardY, TargetX, TargetY> * data)
        : data_(data), map_used_(0) {
        this->init();
    }

    ~MagicBlockSolver() {}

    size_t getSteps() const {
        return this->moves_.size();
    }

    const std::vector<Move> & getMoves() const {
        return this->moves_;
    }

    size_t getMapUsed() const {
        return this->map_used_;
    }

    void setBoard(const Board<BoardX, BoardY> & board) {
        this->board_ = board;
    }

    bool find_empty(const Board<BoardX, BoardY> & board, Position16 & empty_pos) const {
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                uint8_t cell = board.cells[y * BoardY + x];
                if (cell == Color::Empty) {
                    empty_pos.value = (int16_t)(y * BoardY + x);
                    return true;
                }
            }
        }
        return false;
    }

    void count_board_color_nums(const Board<BoardX, BoardY> & board) {
        for (size_t clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->board_colors_[clr] = 0;
        }

        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                char cell = board.cells[y * BoardY + x];
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->board_colors_[cell]++;
                }
            }
        }
    }

    void count_reverse_partial_color_nums(const Board<BoardX, BoardY> & board,
                                          size_t firstX, size_t lastX,
                                          size_t firstY, size_t lastY) {
#ifdef ENABLE_DEBUG
        Board<BoardX, BoardY> test_board;
        for (size_t y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_t x = firstX; x < lastX; x++) {
                test_board.cells[baseY + x] = 1;
            }
        }
#endif
        this->partial_colors_[Color::Empty] = 0;
        for (size_t clr = Color::First; clr <= Color::Last; clr++) {
            this->partial_colors_[clr] = kSingelColorNums;
        }

        for (size_t y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_t x = firstX; x < lastX; x++) {
                uint8_t cell = board.cells[baseY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell <= Color::Last) {
                    this->partial_colors_[cell]--;
                }
            }
        }
    }

    void count_partial_color_nums(const Board<BoardX, BoardY> & board,
                                  size_t firstX, size_t lastX,
                                  size_t firstY, size_t lastY) {
        for (size_t clr = Color::Empty; clr <= Color::Last; clr++) {
            this->partial_colors_[clr] = 0;
        }

        for (size_t y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_t x = firstX; x < lastX; x++) {
                uint8_t cell = board.cells[baseY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell <= Color::Last) {
                    this->partial_colors_[cell]++;
                }
            }
        }
    }

    bool check_partial_color_nums() const {
        if (Step == 456) {
            for (size_t clr = Color::Empty; clr <= Color::Last; clr++) {
                if (this->partial_colors_[clr] < this->data_->target_colors[clr]) {
                    return false;
                }
            }
        }
        else {
            for (size_t clr = Color::First; clr <= Color::Last; clr++) {
                if (this->partial_colors_[clr] < this->data_->target_colors[clr]) {
                    return false;
                }
            }
        }
        return true;
    }

    void translateMoves(const std::vector<Move> & moves) {
        this->moves_ = moves;
    }

    bool is_satisfy_full(const Board<BoardX, BoardY> & board,
                         const Board<TargetX, TargetY> & target) const {
        for (size_t y = 0; y < TargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_t x = 0; x < TargetX; x++) {
                uint8_t target_cell = target.cells[targetBaseY + x];
                uint8_t cell = board.cells[baseY + (startX + x)];
                assert_color(target_cell);
                assert_color(cell);
                if (cell != target_cell) {
                    return false;
                }
            }
        }

        return true;
    }

    bool solve_full() {
        if (is_satisfy_full(this->board_, this->target_)) {
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position16 empty;
        bool found_empty = find_empty(this->board_, empty);
        if (found_empty) {
            stage_type start;
            start.empty = empty;
            start.last_dir = -1;
            start.board = this->data_->board;
            this->visited_.insert(start.board.value128());

            this->cur_.push_back(start);

            bool found = false;
            while (this->cur_.size()) {
                for (size_t i = 0; i < this->cur_.size(); i++) {
                    const stage_type & stage = this->cur_[i];

                    int empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        stage_type next_stage(stage.board);
                        int move_pos = empty_moves[n].pos.value;
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        uint128_t board_value = next_stage.board.value128();
                        if (this->visited_.count(board_value) > 0)
                            continue;

                        this->visited_.insert(board_value);
                        
                        Position16 next_empty(move_pos);
                        next_stage.empty = next_empty;
                        next_stage.last_dir = cur_dir;
                        next_stage.moves = stage.moves;
                        Move next_move;
                        next_move.pos = stage.empty;
                        next_move.dir = cur_dir;
                        next_stage.moves.push_back(next_move);

                        this->next_.push_back(next_stage);

                        if (is_satisfy_full(next_stage.board, this->data_->target)) {
                            this->moves_ = next_stage.moves;
                            assert((depth + 1) == next_stage.moves.size());
                            found = true;
                            break;
                        }  
                    }

                    if (found) {
                        break;
                    }
                }

                depth++;
                printf("depth = %u\n", (uint32_t)depth);
                printf("cur.size() = %u, next.size() = %u\n", (uint32_t)(this->cur_.size()), (uint32_t)(this->next_.size()));
                printf("visited.size() = %u\n\n", (uint32_t)(this->visited_.size()));

                std::swap(this->cur_, this->next_);
                this->next_.clear();

                if (found) {
                    break;
                }
            }

            if (found) {
                solvable = true;
                this->map_used_ = visited_.size();
            }
        }

        return solvable;
    }

    size_t is_satisfy_step_1(const Board<BoardX, BoardY> & board,
                             const Board<TargetX, TargetY> & target) {
        static const ptrdiff_t startX = (BoardX - TargetX) / 2;
        static const ptrdiff_t startY = (BoardY - TargetY) / 2;

        size_t result = 0;

        // Left-Top Corner
        static const ptrdiff_t LeftTopX = startX;
        static const ptrdiff_t LeftTopY = startY;

        if (board.cells[LeftTopY * BoardY + LeftTopX] ==
            target.cells[0 * TargetY + 0]) {
            count_reverse_partial_color_nums(board, 0, LeftTopX + 1, 0, LeftTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 1;
            }
        }

        // Right-Top Corner
        static const ptrdiff_t RightTopX = startX + TargetX - 1;
        static const ptrdiff_t RightTopY = startY;

        if (board.cells[RightTopY * BoardY + RightTopX] ==
            target.cells[0 * TargetY + (TargetX - 1)]) {
            count_reverse_partial_color_nums(board, RightTopX, BoardX, 0, RightTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 2;
            }
        }

        // Left-Bottom Corner
        static const ptrdiff_t LeftBottomX = startX;
        static const ptrdiff_t LeftBottomY = startY + TargetY - 1;

        if (board.cells[LeftBottomY * BoardY + LeftBottomX] ==
            target.cells[(TargetY - 1) * TargetY + 0]) {
            count_reverse_partial_color_nums(board, 0, LeftBottomX + 1, LeftBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 4;
            }
        }

        // Right-Bottom Corner
        static const ptrdiff_t RightBottomX = startX + TargetX - 1;
        static const ptrdiff_t RightBottomY = startY + TargetY - 1;

        if (board.cells[RightBottomY * BoardY + RightBottomX] ==
            target.cells[(TargetY - 1) * TargetY + (TargetX - 1)]) {
            count_reverse_partial_color_nums(board, RightBottomX, BoardX, RightBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 8;
            }
        }

        return result;
    }

    bool verify_board_is_equal(const Board<BoardX, BoardY> & board,
                               const Board<TargetX, TargetY> & target,
                               size_t firstTargetX, size_t lastTargetX,
                               size_t firstTargetY, size_t lastTargetY) {
#ifdef ENABLE_DEBUG
        Board<BoardX, BoardY> test_board;
        for (size_t y = firstTargetY; y < lastTargetY; y++) {
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_t x = firstTargetX; x < lastTargetX; x++) {
                test_board.cells[baseY + (startX + x)] = 1;
            }
        }
#endif

        for (size_t y = firstTargetY; y < lastTargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_t x = firstTargetX; x < lastTargetX; x++) {
                uint8_t target_cell = target.cells[targetBaseY + x];
                uint8_t cell = board.cells[baseY + (startX + x)];
                assert_color(target_cell);
                assert_color(cell);
                if (cell != target_cell) {
                    return false;
                }
            }
        }
        return true;
    }

    size_t is_satisfy_step_12(const Board<BoardX, BoardY> & board,
                              const Board<TargetX, TargetY> & target) {
        size_t result = 0;

        // Left-Top Corner
        static const ptrdiff_t LeftTopX = startX;
        static const ptrdiff_t LeftTopY = startY;

        if (verify_board_is_equal(board, target, 0, 2, 0, 1)) {
            count_reverse_partial_color_nums(board, 0, LeftTopX + 2, 0, LeftTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 1;
            }
        }

        // Right-Top Corner
        static const ptrdiff_t RightTopX = startX + TargetX - 1;
        static const ptrdiff_t RightTopY = startY;

        if (verify_board_is_equal(board, target, TargetX - 2, TargetX, 0, 1)) {
            count_reverse_partial_color_nums(board, RightTopX - 1, BoardX, 0, RightTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 2;
            }
        }

        // Left-Bottom Corner
        static const ptrdiff_t LeftBottomX = startX;
        static const ptrdiff_t LeftBottomY = startY + TargetY - 1;

        if (verify_board_is_equal(board, target, 0, 2, TargetY - 1, TargetY)) {
            count_reverse_partial_color_nums(board, 0, LeftBottomX + 2, LeftBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 4;
            }
        }

        // Right-Bottom Corner
        static const ptrdiff_t RightBottomX = startX + TargetX - 1;
        static const ptrdiff_t RightBottomY = startY + TargetY - 1;

        if (verify_board_is_equal(board, target, TargetX - 2, TargetX, TargetY - 1, TargetY)) {
            count_reverse_partial_color_nums(board, RightBottomX - 1, BoardX, RightBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 8;
            }
        }

        return result;
    }

    size_t is_satisfy_step_123(const Board<BoardX, BoardY> & board,
                               const Board<TargetX, TargetY> & target) {
        size_t result = 0;

        // Top partial
        static const ptrdiff_t TopY = startY;

        if (verify_board_is_equal(board, target, 0, TargetX, 0, 1)) {
            count_reverse_partial_color_nums(board, 0, BoardX, 0, TopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 1;
            }
        }

        // Left partial
        static const ptrdiff_t LeftX = startX;

        if (verify_board_is_equal(board, target, 0, 1, 0, TargetY)) {
            count_reverse_partial_color_nums(board, 0, LeftX + 1, 0, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 2;
            }
        }

        // Right partial
        static const ptrdiff_t RightX = startX + TargetX - 1;

        if (verify_board_is_equal(board, target, TargetX - 1, TargetX, 0, TargetY)) {
            count_reverse_partial_color_nums(board, RightX, BoardX, 0, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 4;
            }
        }

        // Bottom partial
        static const ptrdiff_t BottomY = startY + TargetY - 1;

        if (verify_board_is_equal(board, target, 0, TargetX, TargetY - 1, TargetY)) {
            count_reverse_partial_color_nums(board, 0, BoardX, BottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 8;
            }
        }

        return result;
    }

    size_t is_satisfy_step_456(const Board<BoardX, BoardY> & board,
                               const Board<TargetX, TargetY> & target) {
        size_t result = 0;

        if (this->data_->s456.openning_type == 0) {
            // Top partial
            static const ptrdiff_t TopY = startY;

            if (verify_board_is_equal(board, target, 0, TargetX, 0, 2)) {
                count_partial_color_nums(board, 0, BoardX, TopY + 2, BoardY);
                if (Step == 456 || this->partial_colors_[Color::Empty] == 1) {
                    bool is_valid = check_partial_color_nums();
                    if (is_valid)
                        result |= 1;
                }
            }
        }
        else if (this->data_->s456.openning_type == 1) {
            // Left partial
            static const ptrdiff_t LeftX = startX;

            if (verify_board_is_equal(board, target, 0, 2, 0, TargetY)) {
                count_partial_color_nums(board, LeftX + 2, BoardX, 0, BoardY);
                if (Step == 456 || this->partial_colors_[Color::Empty] == 1) {
                    bool is_valid = check_partial_color_nums();
                    if (is_valid)
                        result |= 2;
                }
            }
        }
        else if (this->data_->s456.openning_type == 2) {
            // Right partial
            static const ptrdiff_t RightX = startX + TargetX - 1;

            if (verify_board_is_equal(board, target, TargetX - 2, TargetX, 0, TargetY)) {
                count_partial_color_nums(board, 0, startX + 1, 0, BoardY);
                if (Step == 456 || this->partial_colors_[Color::Empty] == 1) {
                    bool is_valid = check_partial_color_nums();
                    if (is_valid)
                        result |= 4;
                }
            }
        }
        else if (this->data_->s456.openning_type == 3) {
            // Bottom partial
            static const ptrdiff_t BottomY = startY + TargetY - 1;

            if (verify_board_is_equal(board, target, 0, TargetX, TargetY - 2, TargetY)) {
                count_partial_color_nums(board, 0, BoardX, 0, startY + 1);
                if (Step == 456 || this->partial_colors_[Color::Empty] == 1) {
                    bool is_valid = check_partial_color_nums();
                    if (is_valid)
                        result |= 8;
                }
            }
        }
        else {
            assert(false);
        }

        return result;
    }

    bool record_min_openning(size_t depth, size_t sat_mask, const stage_type & stage) {
#ifndef NDEBUG
        static const size_t kSlideDepth = 1;
        static const size_t kMaxSlideDepth = 2;
#else
        static const size_t kSlideDepth = 6;
        static const size_t kMaxSlideDepth = 10;
#endif
        size_t reached_mask = 0;
        size_t mask = 1;
        size_t type = 0;
        while (sat_mask != 0) {
            if ((sat_mask & mask) == mask) {
                // record min-move openning stage
                this->data_->s123.stage_list[type].push_back(stage);
                if (this->data_->s123.min_depth[type] != -1) {
                    assert(this->data_->s123.max_depth[type] != -1);
                    if (depth >= this->data_->s123.max_depth[type]) {
                        reached_mask |= mask;
                    }
                }
                else {
                    if (this->data_->s123.depth_limit == -1) {
                        this->data_->s123.depth_limit = std::min(std::max(depth + kMaxSlideDepth, 15ULL), 27ULL);
                    }
                    this->data_->s123.min_depth[type] = (int)depth;
                    this->data_->s123.max_depth[type] = (int)(depth + kSlideDepth);
                }
            }
            type++;
            if (type >= 4)
                break;
            sat_mask &= ~mask;
            mask <<= 1;
        }

        return ((reached_mask & 0x0F) == 0x0F);
    }

    size_t is_satisfy(const Board<BoardX, BoardY> & board,
                      const Board<TargetX, TargetY> & target) {
        if (Step == 1) {
            return is_satisfy_step_1(board, target);
        }
        if (Step == 12) {
            return is_satisfy_step_12(board, target);
        }
        else if (Step == 123) {
            return is_satisfy_step_123(board, target);
        }
        else if (Step == 456) {
            return is_satisfy_step_456(board, target);
        }
        else {
            return (size_t)is_satisfy_full(board, target);
        }

        return 0;
    }

    bool solve() {
        size_t sat_mask = is_satisfy(this->board_, this->target_);
        if (sat_mask > 0) {
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position16 empty;
        bool found_empty = find_empty(this->board_, empty);
        if (found_empty) {
            stage_type start;
            start.empty = empty;
            start.last_dir = -1;
            start.board = this->board_;
            this->visited_.insert(start.board.value128());

            this->cur_.push_back(start);

            bool exit = false;
            while (this->cur_.size()) {
                for (size_t i = 0; i < this->cur_.size(); i++) {
                    const stage_type & stage = this->cur_[i];

                    int empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        int move_pos = empty_moves[n].pos.value;
                        if (Step == 456) {
                            if (this->data_->s456.locked[move_pos] > 0)
                                continue;
                        }

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        uint128_t board_value = next_stage.board.value128();
                        if (this->visited_.count(board_value) > 0)
                            continue;

                        this->visited_.insert(board_value);

                        next_stage.empty.value = (int16_t)move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.moves = stage.moves;
                        Move next_move;
                        next_move.pos = stage.empty;
                        next_move.dir = cur_dir;
                        next_stage.moves.push_back(next_move);

                        this->next_.push_back(next_stage);

                        sat_mask = is_satisfy(next_stage.board, this->target_);
                        if (sat_mask > 0) {
                            solvable = true;
                            if (Step == 1 || Step == 12 || Step == 123) {
                                bool reached_end = record_min_openning(depth, sat_mask, next_stage);
                                if (reached_end) {
                                    exit = true;
                                }
                            }
                            else {
                                this->moves_ = next_stage.moves;
                                assert((depth + 1) == next_stage.moves.size());
                                exit = true;
                                break;
                            }
                        }
                    }
                    if (!(Step == 1 || Step == 12 || Step == 123)) {
                        if (exit) {
                            break;
                        }
                    }
                }

                depth++;
                printf("depth = %u\n", (uint32_t)depth);
                printf("cur.size() = %u, next.size() = %u\n",
                       (uint32_t)(this->cur_.size()), (uint32_t)(this->next_.size()));
                printf("visited.size() = %u\n\n", (uint32_t)(this->visited_.size()));

                std::swap(this->cur_, this->next_);
                this->next_.clear();

                if (exit) {
                    break;
                }

                if (Step == 1 || Step == 12 || Step == 123) {
                    if (this->data_->s123.depth_limit != -1 && depth >= this->data_->s123.depth_limit) {
                        exit = true;
                        break;
                    }
                }
                else if (Step == 456) {
                    if (depth >= 32) {
                        exit = true;
                        break;
                    }
                }
            }

            this->map_used_ = visited_.size();
            printf("sat_mask = %u\n\n", (uint32_t)sat_mask);

            if (Step == 1 || Step == 12 || Step == 123) {
                printf("Solvable: %s\n", (solvable ? "true" : "false"));
                for (size_t i = 0; i < 4; i++) {
                    printf("i = %u, min_depth = %d, max_depth = %d, stage.size() = %u\n",
                            (uint32_t)(i + 1),
                            this->data_->s123.min_depth[i],
                            this->data_->s123.max_depth[i],
                            (uint32_t)this->data_->s123.stage_list[i].size());
                }
                printf("\n");
            }
            else if (Step == 456) {
                printf("Step: 456\n\n");
                printf("Solvable: %s\n", (solvable ? "true" : "false"));
                printf("OpenningType = %u\n", (uint32_t)this->data_->s456.openning_type);
                printf("Index = %u\n", (uint32_t)this->data_->s456.index);
                printf("next.size() = %u\n", (uint32_t)this->cur_.size());
                printf("moves.size() = %u\n", (uint32_t)this->moves_.size());
                printf("\n");
            }
        }

        return solvable;
    }
};

} // namespace PuzzleGame
