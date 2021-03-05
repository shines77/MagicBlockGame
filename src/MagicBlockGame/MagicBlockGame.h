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
#include "MagicBlockSolver.h"
#include "SlidingPuzzle.h"

namespace PuzzleGame {

template <size_t BoardX, size_t BoardY,
          size_t TargetX, size_t TargetY>
class MagicBlockGame
{
public:
    static const size_t kSingelColorNums = (BoardX * BoardY - 1) / (Color::Last - 1);

    static const ptrdiff_t startX = (BoardX - TargetX) / 2;
    static const ptrdiff_t startY = (BoardY - TargetY) / 2;

    typedef typename SharedData<BoardX, BoardY, TargetX, TargetY>::stage_type stage_type;

    typedef MagicBlockSolver<BoardX, BoardY, TargetX, TargetY, 1>   Step1Solver;
    typedef MagicBlockSolver<BoardX, BoardY, TargetX, TargetY, 123> Step123Solver;
    typedef MagicBlockSolver<BoardX, BoardY, TargetX, TargetY, 456> Step456Solver;

private:
    SharedData<BoardX, BoardY, TargetX, TargetY> data_;

    size_t min_steps_;
    size_t map_used_;

    std::vector<Position> move_path_;
    std::vector<Position> answer_;

    void init() {
        // Initialize empty_moves[BoardX * BoardY]
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                std::vector<Move> empty_moves;
                for (size_t dir = Direction::First; dir < Direction::Last; dir++) {
                    int board_x = (int)x + Dir_Offset[dir].x;
                    if (board_x < 0 || board_x >= (int)BoardX)
                        continue;
                    int board_y = (int)y + Dir_Offset[dir].y;
                    if (board_y < 0 || board_y >= (int)BoardY)
                        continue;
                    Move move;
                    move.pos = Position(board_y * BoardY + board_x);
                    move.dir = (uint8_t)dir;
                    empty_moves.push_back(move);
                }
                this->data_.empty_moves[y * BoardY + x] = empty_moves;
            }
        }
    }

public:
    MagicBlockGame() : min_steps_(size_t(-1)), map_used_(0) {
        this->init();
    }

    ~MagicBlockGame() {}

    size_t getSteps() const {
        return this->move_path_.size();
    }

    const std::vector<Position> & getMovePath() const {
        return this->move_path_;
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
                            if (color >= Color::Empty && color < Color::Last) {
                                this->data_.target.cells[line_no * TargetY + x] = color;
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
                            if (color >= Color::Empty && color < Color::Last) {
                                this->data_.board.cells[boardY * BoardY + x] = color;
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

        if (result == 1) {
            count_color_nums();
        }

        return result;
    }

    void count_color_nums() {
        for (size_t clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->data_.board_colors[clr] = 0;
            this->data_.target_colors[clr] = 0;
        }

        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                char cell = this->data_.board.cells[y * BoardY + x];
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->data_.board_colors[cell]++;
                }
            }
        }

        for (size_t y = 0; y < TargetY; y++) {
            for (size_t x = 0; x < TargetX; x++) {
                char cell = this->data_.target.cells[y * TargetY + x];
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->data_.target_colors[cell]++;
                }
            }
        }
    }

    bool find_empty(const Board<BoardX, BoardY> & board, Position & empty_pos) const {
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

    bool is_satisfy(const Board<BoardX, BoardY> & board,
                    const Board<TargetX, TargetY> & target) const {
        for (size_t y = 0; y < TargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_t x = 0; x < TargetX; x++) {
                uint8_t target = this->data_.target.cells[targetBaseY + x];
                uint8_t disc = this->data_.board.cells[baseY + (startX + x)];
                if (disc != target) {
                    return false;
                }
            }
        }

        return true;
    }

    void translateMovePath(const std::vector<Position> & move_path) {
        this->move_path_ = move_path;
    }

    bool solve() {
        if (is_satisfy(this->data_.board, this->data_.target)) {
            return true;
        }

        bool solvable = false;

        Position empty;
        bool found_empty = find_empty(this->data_.board, empty);
        if (found_empty) {
            Step123Solver solver_123(&this->data_);
            solvable = solver_123.solve();

            if (solvable) {
                for (size_t i = 0; i < 4; i++) {
                    this->data_.s456.openning_type = i;
                    const std::vector<stage_type> & stage_list = this->data_.s123.stage_list[i];
                    size_t totalStage = stage_list.size();
                    for (size_t n = 0; n < totalStage; n++) {
                        this->data_.s456.index = n;
                        if (this->min_steps_ > stage_list[n].move_path.size()) {
                            this->data_.s456.depth_limit = std::min(35ULL,
                                this->min_steps_ - stage_list[n].move_path.size());
                        }
                        else {
                            continue;
                        }
                        Step456Solver solver_456(&this->data_);
                        solver_456.setBoard(stage_list[n].board);
                        solvable = solver_456.solve();
                        if (solvable) {
                            this->move_path_ = solver_456.getMovePath();
                            size_t total_steps = stage_list[n].move_path.size() + this->move_path_.size();
                            printf("Step123 moves: %u, Step456 moves: %u, Total moves: %u\n\n",
                                   (uint32_t)stage_list[n].move_path.size(),
                                   (uint32_t)this->move_path_.size(),
                                   (uint32_t)total_steps);

                            if (total_steps < this->min_steps_) {
                                this->map_used_ = solver_456.getMapUsed();
                                this->min_steps_ = total_steps;
                                this->answer_ = stage_list[n].move_path;
                                for (auto iter : this->move_path_) {
                                    this->answer_.push_back(iter);
                                }
                                printf("Total moves: %u\n\n", (uint32_t)this->answer_.size());
                            }
                        }
                    }
                }

                printf("this->min_steps: %u\n", (uint32_t)this->min_steps_);
                printf("Total moves: %u\n", (uint32_t)this->answer_.size());
                printf("\n");

                if (this->min_steps_ != size_t(-1) || this->answer_.size() > 0) {
                    solvable = true;
                    this->move_path_ = this->answer_;
                }
            }
        }

        return solvable;
    }

    bool solve_3x3() {
        SlidingPuzzle<TargetX, TargetY> slidingPuzzle;
        slidingPuzzle.setPuzzle<BoardX, BoardY>(this->data_.board, this->data_.target);
        bool solvable = slidingPuzzle.solve();
        if (solvable) {
            translateMovePath(slidingPuzzle.getMovePath());
            this->map_used_ = slidingPuzzle.getMapUsed();
        }
        return solvable;
    }
};

} // namespace PuzzleGame
