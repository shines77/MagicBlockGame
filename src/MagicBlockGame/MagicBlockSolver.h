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
#include <algorithm>    // For std::swap(), until C++11
#include <utility>      // For std::swap(), since C++11

#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "SharedData.h"
#include "SlidingPuzzle.h"

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

    struct State {
        Position16  empty;
        uint8_t     last_dir, reserve;
        Board<BoardX, BoardY> board;
        std::vector<Move> moves;

        State() {}
        State(const Board<BoardX, BoardY> & srcBoard) {
            this->board = srcBoard;
        }
    };

private:
    const SharedData<BoardX, BoardY, TargetX, TargetY> * data_;

    Board<BoardX, BoardY> board_;
    Board<TargetX, TargetY> target_;

    int board_colors_[Color::Maximum];
    int partial_colors_[Color::Maximum];
    int target_colors_[Color::Maximum];

    std::vector<State> cur_;
    std::vector<State> next_;

    std::set<uint128_t> visited_;

    const std::vector<Move> * empty_moves_;
    std::vector<Move> moves_;

    size_t map_used_;

    void init() {
        this->board_ = this->data_->board;
        this->target_ = this->data_->target;

        count_board_color_nums(this->board_);
    }

public:
    MagicBlockSolver(const SharedData<BoardX, BoardY, TargetX, TargetY> * data)
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

    bool find_empty(const Board<BoardX, BoardY> & board, Position16 & empty_pos) const {
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                char color = board.cells[y * BoardY + x];
                if (color == Color::Empty) {
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

    void count_partial_color_nums(const Board<BoardX, BoardY> & board,
                                  size_t firstX, size_t lastX,
                                  size_t firstY, size_t lastY) {
        this->partial_colors_[Color::Empty] = 0;
        for (size_t clr = Color::First; clr <= Color::Last; clr++) {
            this->partial_colors_[clr] = kSingelColorNums;
        }

        for (size_t y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_t x = firstX; x < lastX; x++) {
                uint8_t cell = board.cells[baseY + x];
                if (cell >= Color::Empty && cell <= Color::Last) {
                    this->partial_colors_[cell]--;
                }
            }
        }
    }

    bool check_partial_color_nums() const {
        for (size_t clr = Color::First; clr <= Color::Last; clr++) {
            if (this->partial_colors_[clr] < this->data_->target_colors[clr]) {
                return false;
            }
        }
        return true;
    }

    void translateMoves(const std::vector<Move> & moves) {
        this->moves_ = moves;
    }

    bool is_satisfy(const Board<BoardX, BoardY> & board,
                    const Board<TargetX, TargetY> & target) const {
        if (Step == 1) {
            return is_satisfy_step_01(board, target);
        }
        else if (Step == 123) {
            return is_satisfy_step_123(board, target);
        }
        else {
            return is_satisfy_full(board, target);
        }

        return false;
    }

    bool is_satisfy_full(const Board<BoardX, BoardY> & board,
                         const Board<TargetX, TargetY> & target) const {
        for (size_t y = 0; y < TargetY; y++) {
            ptrdiff_t targetBaseY = y * BoardY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_t x = 0; x < TargetX; x++) {
                uint8_t target_cell = target.cells[targetBaseY + x];
                uint8_t cell = board.cells[baseY + (startX + x)];
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
            State first;
            first.empty = empty;
            first.last_dir = -1;
            first.board = this->data_->board;
            this->visited_.insert(first.board.value128());

            this->cur_.push_back(first);

            bool found = false;
            while (this->cur_.size()) {
                for (size_t i = 0; i < this->cur_.size(); i++) {
                    const State & state = this->cur_[i];

                    int empty_pos = state.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        if (empty_moves[n].dir == state.last_dir)
                            continue;

                        State next_state(state.board);
                        int move_pos = empty_moves[n].pos.value;
                        std::swap(next_state.board.cells[empty_pos], next_state.board.cells[move_pos]);
                        uint128_t board_value = next_state.board.value128();
                        if (this->visited_.count(board_value) > 0)
                            continue;

                        this->visited_.insert(board_value);
                        
                        Position16 next_empty(move_pos);
                        next_state.empty = next_empty;
                        next_state.last_dir = (uint8_t)n;
                        next_state.moves = state.moves;
                        Move next_move;
                        next_move.pos = state.empty;
                        next_move.dir = (uint8_t)n;
                        next_state.moves.push_back(next_move);

                        if (is_satisfy_full(next_state.board, this->data_->target)) {
                            this->moves_ = next_state.moves;
                            assert((depth + 1) == next_state.moves.size());
                            found = true;
                            break;
                        }

                        this->next_.push_back(next_state);
                    }

                    if (found) {
                        break;
                    }
                }

                if (found) {
                    break;
                }

                depth++;
                printf("depth = %u\n", (uint32_t)(depth + 1));
                printf("cur.size() = %u, next.size() = %u\n", (uint32_t)(this->cur_.size()), (uint32_t)(this->next_.size()));
                printf("visited.size() = %u\n\n", (uint32_t)(this->visited_.size()));

                std::swap(this->cur_, this->next_);
                this->next_.clear();
            }

            if (found) {
                solvable = true;
                this->map_used_ = visited_.size();
            }
        }

        return solvable;
    }

    uint32_t is_satisfy_step_01(const Board<BoardX, BoardY> & board,
                                const Board<TargetX, TargetY> & target) {
        static const ptrdiff_t startX = (BoardX - TargetX) / 2;
        static const ptrdiff_t startY = (BoardY - TargetY) / 2;

        uint32_t result = 0;

        // Left-Top Corner
        static const ptrdiff_t LeftTopX = startX;
        static const ptrdiff_t LeftTopY = startY;

        if (board.cells[LeftTopY * BoardY + LeftTopX] ==
            target.cells[0 * TargetY + 0]) {
            count_partial_color_nums(board, 0, LeftTopX + 1, 0, LeftTopY + 1);
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
            count_partial_color_nums(board, RightTopX, BoardX, 0, RightTopY + 1);
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
            count_partial_color_nums(board, 0, LeftBottomX + 1, LeftBottomY, BoardY);
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
            count_partial_color_nums(board, RightBottomX, BoardX, RightBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 8;
            }
        }

        return result;
    }

    bool solve_step_01() {
        uint32_t sat_mask = is_satisfy_step_01(this->board_, this->target_);
        if (sat_mask > 0) {
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position16 empty;
        bool found_empty = find_empty(this->board_, empty);
        if (found_empty) {
            State first;
            first.empty = empty;
            first.last_dir = -1;
            first.board = this->board_;
            this->visited_.insert(first.board.value128());

            this->cur_.push_back(first);

            bool found = false;
            while (this->cur_.size()) {
                for (size_t i = 0; i < this->cur_.size(); i++) {
                    const State & state = this->cur_[i];

                    int empty_pos = state.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        if (empty_moves[n].dir == state.last_dir)
                            continue;

                        State next_state(state.board);
                        int move_pos = empty_moves[n].pos.value;
                        std::swap(next_state.board.cells[empty_pos], next_state.board.cells[move_pos]);
                        uint128_t board_value = next_state.board.value128();
                        if (this->visited_.count(board_value) > 0)
                            continue;

                        this->visited_.insert(board_value);
                        
                        Position16 next_empty(move_pos);
                        next_state.empty = next_empty;
                        next_state.last_dir = (uint8_t)n;
                        next_state.moves = state.moves;
                        Move next_move;
                        next_move.pos = state.empty;
                        next_move.dir = (uint8_t)n;
                        next_state.moves.push_back(next_move);

                        sat_mask = is_satisfy_step_01(next_state.board, this->target_);
                        if (sat_mask > 0) {
                            this->moves_ = next_state.moves;
                            assert((depth + 1) == next_state.moves.size());
                            found = true;
                            break;
                        }

                        this->next_.push_back(next_state);
                    }

                    if (found) {
                        break;
                    }
                }

                if (found) {
                    break;
                }

                depth++;
                printf("depth = %u\n", (uint32_t)(depth + 1));
                printf("cur.size() = %u, next.size() = %u\n", (uint32_t)(this->cur_.size()), (uint32_t)(this->next_.size()));
                printf("visited.size() = %u\n\n", (uint32_t)(this->visited_.size()));

                std::swap(this->cur_, this->next_);
                this->next_.clear();
            }

            if (found) {
                solvable = true;
                this->map_used_ = visited_.size();
                printf("sat_mask_01 = %d\n\n", sat_mask);
            }
        }

        return solvable;
    }

    bool verify_board_is_equal(const Board<BoardX, BoardY> & board,
                               const Board<TargetX, TargetY> & target,
                               size_t firstTargetX, size_t lastTargetX,
                               size_t firstTargetY, size_t lastTargetY) {
        for (size_t y = firstTargetY; y < lastTargetY; y++) {
            ptrdiff_t targetBaseY = y * BoardY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_t x = firstTargetX; x < lastTargetX; x++) {
                uint8_t target_cell = target.cells[targetBaseY + x];
                uint8_t cell = board.cells[baseY + (startX + x)];
                if (cell != target_cell) {
                    return false;
                }
            }
        }
        return true;
    }

#if 1

    uint32_t is_satisfy_step_123(const Board<BoardX, BoardY> & board,
                                 const Board<TargetX, TargetY> & target) {
        uint32_t result = 0;

        // Top partial
        static const ptrdiff_t TopY = startY;

        if (verify_board_is_equal(board, target, 0, TargetX, 0, 1)) {
            count_partial_color_nums(board, 0, BoardX, 0, TopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 1;
            }
        }

        // Left partial
        static const ptrdiff_t LeftX = startX;

        if (verify_board_is_equal(board, target, 0, 1, 0, TargetY)) {
            count_partial_color_nums(board, 0, LeftX + 1, 0, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 2;
            }
        }

        // Right partial
        static const ptrdiff_t RightX = startX + TargetX - 1;

        if (verify_board_is_equal(board, target, TargetX - 1, TargetX, 0, TargetY)) {
            count_partial_color_nums(board, RightX, BoardX, 0, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 4;
            }
        }

        // Bottom partial
        static const ptrdiff_t BottomY = startY + TargetY - 1;

        if (verify_board_is_equal(board, target, 0, TargetX, TargetY - 1, TargetY)) {
            count_partial_color_nums(board, 0, BoardX, BottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 8;
            }
        }

        return result;
    }

#else

    uint32_t is_satisfy_step_123(const Board<BoardX, BoardY> & board,
                                 const Board<TargetX, TargetY> & target) {
        uint32_t result = 0;

        // Left-Top Corner
        static const ptrdiff_t LeftTopX = startX;
        static const ptrdiff_t LeftTopY = startY;

        if (verify_board_is_equal(board, target, 0, 2, 0, 1)) {
            count_partial_color_nums(board, 0, LeftTopX + 2, 0, LeftTopY + 1);
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
            count_partial_color_nums(board, RightTopX - 1, BoardX, 0, RightTopY + 1);
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
            count_partial_color_nums(board, 0, LeftBottomX + 2, LeftBottomY, BoardY);
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
            count_partial_color_nums(board, RightBottomX - 1, BoardX, RightBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    result |= 8;
            }
        }

        return result;
    }

#endif

    bool solve_step_123() {
        uint32_t sat_mask = is_satisfy_step_123(this->board_, this->target_);
        if (sat_mask > 0) {
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position16 empty;
        bool found_empty = find_empty(this->board_, empty);
        if (found_empty) {
            State first;
            first.empty = empty;
            first.last_dir = -1;
            first.board = this->board_;
            this->visited_.insert(first.board.value128());

            this->cur_.push_back(first);

            bool found = false;
            while (this->cur_.size()) {
                for (size_t i = 0; i < this->cur_.size(); i++) {
                    const State & state = this->cur_[i];

                    int empty_pos = state.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        if (empty_moves[n].dir == state.last_dir)
                            continue;

                        State next_state(state.board);
                        int move_pos = empty_moves[n].pos.value;
                        std::swap(next_state.board.cells[empty_pos], next_state.board.cells[move_pos]);
                        uint128_t board_value = next_state.board.value128();
                        if (this->visited_.count(board_value) > 0)
                            continue;

                        this->visited_.insert(board_value);
                        
                        Position16 next_empty(move_pos);
                        next_state.empty = next_empty;
                        next_state.last_dir = (uint8_t)n;
                        next_state.moves = state.moves;
                        Move next_move;
                        next_move.pos = state.empty;
                        next_move.dir = (uint8_t)n;
                        next_state.moves.push_back(next_move);

                        sat_mask = is_satisfy_step_123(next_state.board, this->target_);
                        if (sat_mask > 0) {
                            this->moves_ = next_state.moves;
                            assert((depth + 1) == next_state.moves.size());
                            found = true;
                            break;
                        }

                        this->next_.push_back(next_state);
                    }

                    if (found) {
                        break;
                    }
                }

                if (found) {
                    break;
                }

                depth++;
                printf("depth = %u\n", (uint32_t)(depth + 1));
                printf("cur.size() = %u, next.size() = %u\n", (uint32_t)(this->cur_.size()), (uint32_t)(this->next_.size()));
                printf("visited.size() = %u\n\n", (uint32_t)(this->visited_.size()));

                std::swap(this->cur_, this->next_);
                this->next_.clear();
            }

            if (found) {
                solvable = true;
                this->map_used_ = visited_.size();
                printf("sat_mask_123 = %d\n\n", sat_mask);
            }
        }

        return solvable;
    }
};

} // namespace PuzzleGame
