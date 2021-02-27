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
#include "SlidingPuzzle.h"

namespace PuzzleGame {

template <size_t BoardX, size_t BoardY, size_t TargetX, size_t TargetY>
class MagicBlockGame
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
    Board<BoardX, BoardY> board_;
    Board<TargetX, TargetY> target_;

    std::vector<State> cur_;
    std::vector<State> next_;

    std::set<uint128_t> visited_;

    std::vector<Move> empty_moves_[BoardX * BoardY];
    std::vector<Move> moves_;

    size_t map_used_;

    void init() {
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                std::vector<Move> moves;
                for (size_t dir = Direction::First; dir < Direction::Last; dir++) {
                    int board_x = (int)x + Dir_Offset[dir].x;
                    if (board_x < 0 || board_x >= (int)BoardX)
                        continue;
                    int board_y = (int)y + Dir_Offset[dir].y;
                    if (board_y < 0 || board_y >= (int)BoardY)
                        continue;
                    Move move;
                    move.pos = Position16(board_y * BoardY + board_x);
                    move.dir = (uint8_t)dir;
                    moves.push_back(move);
                }
                this->empty_moves_[y * BoardY + x] = moves;
            }
        }
    }

public:
    MagicBlockGame() : map_used_(0) {
        this->init();
    }

    ~MagicBlockGame() {}

    size_t getSteps() const {
        return this->moves_.size();
    }

    const std::vector<Move> & getMoves() const {
        return this->moves_;
    }

    size_t getMapUsed() const {
        return this->map_used_;
    }

    int readInput(const char * filename) {
        int result = -1;
        int line_no = 0;
        std::ifstream ifs;
        try {
            ifs.open(filename, std::ios::in);
            if (ifs.good()) {
                result = 0;
                do { 
                    char line[256];
                    std::fill_n(line, sizeof(line), 0);
                    ifs.getline(line, 256);
                    if (line_no >= 0 && line_no < TargetY) {
                        for (size_t x = 0; x < TargetX; x++) {
                            char color = Color::valToColor(line[x]);
                            if (color >= Color::Empty && color < Color::Maximum) {
                                this->target_.cells[line_no * TargetY + x] = color;
                            }
                            else {
                                result = -2;
                                break;
                            }
                        }
                    }
                    else if (line_no >= (TargetY + 1) && line_no < (TargetY + 1 + BoardY)) {
                        size_t boardY = line_no - (TargetY + 1);
                        for (size_t x = 0; x < BoardX; x++) {
                            char color = Color::valToColor(line[x]);
                            if (color >= Color::Empty && color < Color::Maximum) {
                                this->board_.cells[boardY * BoardY + x] = color;
                            }
                            else {
                                result = -3;
                                break;
                            }
                        }
                    }
                    if (result < 0)
                        break;
                    line_no++;
                } while (!ifs.eof());

                ifs.close();

                if (result == 0) {
                    result = 1;
                }
                else {
                    char err_info[256] = {0};
                    snprintf(err_info, sizeof(err_info) - 1, "MagicBlockGame::readInput() Error code = %d", result);
                    throw std::runtime_error(err_info);
                }
            }
        }
        catch (std::exception & ex) {
            std::cout << "Exception: " << ex.what() << std::endl << std::endl;
        }

        return result;
    }

    bool find_empty(Position16 & empty_pos) const {
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                char color = this->board_.cells[y * BoardY + x];
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
        static const ptrdiff_t startX = (BoardX - TargetX) / 2;
        static const ptrdiff_t startY = (BoardY - TargetY) / 2;

        for (size_t y = 0; y < TargetY; y++) {
            ptrdiff_t baseY = (startY + y) * BoardY;
            ptrdiff_t targetBaseY = y * BoardY;
            for (size_t x = 0; x < TargetX; x++) {
                uint8_t target = this->target_.cells[targetBaseY + x];
                uint8_t disc = this->board_.cells[baseY + (startX + x)];
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

    bool solve() {
        if (is_satisfy(this->board_, this->target_)) {
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
            first.board = this->board_;
            this->visited_.insert(first.board.value128());

            this->cur_.push_back(first);

            bool found = false;
            while (this->cur_.size()) {
                for (size_t i = 0; i < this->cur_.size(); i++) {
                    const State & state = this->cur_[i];

                    int empty_pos = state.empty.value;
                    const std::vector<Move> & empty_moves = this->empty_moves_[empty_pos];
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

                        if (is_satisfy(next_state.board ,this->target_)) {
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
                printf("depth = %d\n", (int)(depth + 1));
                printf("cur_.size() = %d, next_.size() = %d\n", (int)(this->cur_.size()), (int)(this->next_.size()));
                printf("visited.size() = %d\n\n", (int)(this->visited_.size()));

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

    bool solve_slow() {
        SlidingPuzzle<TargetX, TargetY> slidingPuzzle;
        slidingPuzzle.setPuzzle<BoardX, BoardY>(this->board_, this->target_);
        bool solvable = slidingPuzzle.solve();
        if (solvable) {
            translateMoves(slidingPuzzle.getMoves());
            this->map_used_ = slidingPuzzle.getMapUsed();
        }
        return solvable;
    }
};

} // namespace PuzzleGame
