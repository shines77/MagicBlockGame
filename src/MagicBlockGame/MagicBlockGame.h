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

    typedef MagicBlockSolver<BoardX, BoardY, TargetX, TargetY, 1> Step01Solver;

private:
    SharedData<BoardX, BoardY, TargetX, TargetY> data_;

    std::vector<Move> moves_;
    size_t map_used_;

    void init() {
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
                    move.pos = Position16(board_y * BoardY + board_x);
                    move.dir = (uint8_t)dir;
                    empty_moves.push_back(move);
                }
                this->data_.empty_moves[y * BoardY + x] = empty_moves;
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
                char color = this->data_.board.cells[y * BoardY + x];
                if (color >= Color::Empty && color < Color::Maximum) {
                    this->data_.board_colors[color]++;
                }
            }
        }

        for (size_t y = 0; y < TargetY; y++) {
            for (size_t x = 0; x < TargetX; x++) {
                char color = this->data_.target.cells[y * BoardY + x];
                if (color >= Color::Empty && color < Color::Maximum) {
                    this->data_.board_colors[color]++;
                }
            }
        }
    }

    bool find_empty(Position16 & empty_pos) const {
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                char color = this->data_.board.cells[y * BoardY + x];
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
                uint8_t target = this->data_.target.cells[targetBaseY + x];
                uint8_t disc = this->data_.board.cells[baseY + (startX + x)];
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
        if (is_satisfy(this->data_.board, this->data_.target)) {
            return true;
        }

        bool solvable = false;

        Position16 empty;
        bool found_empty = find_empty(empty);
        if (found_empty) {
            Step01Solver solver(&this->data_);
            solvable = solver.solve_full();
        }

        return solvable;
    }

    bool solve_3x3() {
        SlidingPuzzle<TargetX, TargetY> slidingPuzzle;
        slidingPuzzle.setPuzzle<BoardX, BoardY>(this->data_.board, this->data_.target);
        bool solvable = slidingPuzzle.solve();
        if (solvable) {
            translateMoves(slidingPuzzle.getMoves());
            this->map_used_ = slidingPuzzle.getMapUsed();
        }
        return solvable;
    }
};

} // namespace PuzzleGame
