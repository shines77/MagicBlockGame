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

        count_board_color_nums();
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

    void count_board_color_nums() {
        for (size_t clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->board_colors_[clr] = 0;
        }

        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                char color = this->board_.cells[y * BoardY + x];
                if (color >= Color::Empty && color < Color::Maximum) {
                    this->board_colors_[color]++;
                }
            }
        }
    }

    bool find_empty(Position16 & empty_pos) const {
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                char color = this->data_->board.cells[y * BoardY + x];
                if (color == Color::Empty) {
                    empty_pos.value = (int16_t)(y * BoardY + x);
                    return true;
                }
            }
        }
        return false;
    }

    bool is_satisfy(const Board<BoardX, BoardY> & board,
                    const Board<TargetX, TargetY> & target) const {
        if (Step == 1) {
            return is_satisfy_step01(board, target);
        }

        return false;
    }

    bool is_satisfy_full(const Board<BoardX, BoardY> & board,
                         const Board<TargetX, TargetY> & target) const {
        static const ptrdiff_t startX = (BoardX - TargetX) / 2;
        static const ptrdiff_t startY = (BoardY - TargetY) / 2;

        for (size_t y = 0; y < TargetY; y++) {
            ptrdiff_t baseY = (startY + y) * BoardY;
            ptrdiff_t targetBaseY = y * BoardY;
            for (size_t x = 0; x < TargetX; x++) {
                uint8_t target = this->data_->target.cells[targetBaseY + x];
                uint8_t disc = this->data_->board.cells[baseY + (startX + x)];
                if (disc != target) {
                    return false;
                }
            }
        }

        return true;
    }

    bool is_satisfy_step01(const Board<BoardX, BoardY> & board,
                           const Board<TargetX, TargetY> & target) const {
        static const ptrdiff_t startX = (BoardX - TargetX) / 2;
        static const ptrdiff_t startY = (BoardY - TargetY) / 2;

        // Left-Up Corner
        static const ptrdiff_t LeftUpX = startX;
        static const ptrdiff_t LeftUpY = startY;

        // Right-Up Corner
        static const ptrdiff_t RightUpX = startX + TargetX - 1;
        static const ptrdiff_t RightUpY = startY;

        // Left-Down Corner
        static const ptrdiff_t LeftDownX = startX;
        static const ptrdiff_t LeftDownY = startY + TargetY - 1;

        // Right-Down Corner
        static const ptrdiff_t LeftUpX = startX + TargetX - 1;
        static const ptrdiff_t LeftUpY = startY + TargetY - 1;

        for (size_t y = 0; y < TargetY; y++) {
            ptrdiff_t baseY = (startY + y) * BoardY;
            ptrdiff_t targetBaseY = y * BoardY;
            for (size_t x = 0; x < TargetX; x++) {
                uint8_t target = this->data_->target.cells[targetBaseY + x];
                uint8_t disc = this->data_->board.cells[baseY + (startX + x)];
                if (disc != target) {
                    return false;
                }
            }
        }

        return true;
    }

    void translateMoves(const std::vector<Move> & moves) {
        this->moves_ = moves;
    }

    bool solve_full() {
        if (is_satisfy_full(this->data_->board, this->data_->target)) {
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position16 empty;
        bool found_empty = find_empty(empty);
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

                        if (is_satisfy_full(next_state.board ,this->data_->target)) {
                            this->moves_ = next_state.moves;
                            assert((depth + 1) == next_state.moves.size());
                            found = true;
                            break;
                        }

                        this->next_.push_back(next_state);
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
};

} // namespace PuzzleGame
