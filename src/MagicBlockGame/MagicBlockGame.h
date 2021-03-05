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
    std::vector<Position> best_move_path_;
    std::vector<MoveInfo> answer_;

    void assert_color(uint8_t color) const {
        assert(color >= Color::Empty && color < Color::Last);
    }

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
                    move.pos = Position(board_y * (int)BoardY + board_x);
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

    size_t getMinSteps() const {
        return this->best_move_path_.size();
    }

    const std::vector<Position> & getMovePath() const {
        return this->move_path_;
    }

    const std::vector<Position> & getBestMovePath() const {
        return this->best_move_path_;
    }

    const std::vector<MoveInfo> & getAnswer() const {
        return this->answer_;
    }

    size_t getMapUsed() const {
        return this->map_used_;
    }

    int readInput(const char * filename) {
        int result = -1;
        size_t line_no = 0;
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
                            uint8_t color = Color::valToColor(line[x]);
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
                            uint8_t color = Color::valToColor(line[x]);
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
                uint8_t cell = this->data_.board.cells[y * BoardY + x];
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->data_.board_colors[cell]++;
                }
            }
        }

        for (size_t y = 0; y < TargetY; y++) {
            for (size_t x = 0; x < TargetX; x++) {
                uint8_t cell = this->data_.target.cells[y * TargetY + x];
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
                uint8_t target_cell = this->data_.target.cells[targetBaseY + x];
                uint8_t cell = this->data_.board.cells[baseY + (startX + x)];
                if (cell != target_cell) {
                    return false;
                }
            }
        }

        return true;
    }

    // Check order: up to down
    bool check_board_is_equal(const Board<BoardX, BoardY> & board,
                              const Board<TargetX, TargetY> & target,
                              size_t firstTargetX, size_t lastTargetX,
                              size_t firstTargetY, size_t lastTargetY) {
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

    bool translateMovePath(const std::vector<Position> & move_path) {
        bool success = true;

        //printf("translateMovePath() begin ...\n\n");
        this->answer_.clear();

        Board<BoardX, BoardY> board(this->data_.board);
        size_t index = 0;
        int16_t from_pos, move_to_pos;
        uint8_t last_dir = uint8_t(-1);
        if (move_path.size() > 0) {
            index++;
            move_to_pos = move_path[0].value;
            uint8_t move_to_cell = board.cells[move_to_pos];
            assert_color(move_to_cell);
            if (move_to_cell == Color::Empty) {
                for (size_t i = 1; i < move_path.size(); i++) {
                    from_pos = move_path[i].value;
                    uint8_t from_cell = board.cells[from_pos];
                    assert_color(from_cell);
                    if (from_cell != Color::Empty) {
                        last_dir = Direction::template getDir<BoardX, BoardY>(from_pos, move_to_pos);
                        MoveInfo move_info;
                        move_info.from_pos = from_pos;
                        move_info.move_to_pos = move_to_pos;
                        move_info.dir = last_dir;
                        this->answer_.push_back(move_info);

                        std::swap(board.cells[from_pos], board.cells[move_to_pos]);
                    }
                    else {
                        printf("translateMovePath():\n\n"
                               "Move path have error, [from_pos] is a empty gird.\n"
                               "index = %u, from_pos = (%u, %u), color = %u\n",
                               (uint32_t)(index + 1), (uint32_t)(from_pos / BoardY),
                               (uint32_t)(from_pos % BoardY), (uint32_t)from_cell);
                        success = false;
                        break;
                    }
                    move_to_pos = from_pos;
                    index++;
                }

                if (success) {
                    bool foundLastStep = false;
                    move_to_cell = board.cells[move_to_pos];
                    assert_color(move_to_cell);
                    if (move_to_cell == Color::Empty) {
                        const std::vector<Move> & empty_moves = this->data_.empty_moves[move_to_pos];
                        size_t total_moves = empty_moves.size();
                        for (size_t n = 0; n < total_moves; n++) {
                            uint8_t cur_dir = empty_moves[n].dir;
                            if (cur_dir == last_dir)
                                continue;

                            from_pos = empty_moves[n].pos.value;
                            std::swap(board.cells[from_pos], board.cells[move_to_pos]);

                            if (check_board_is_equal(board, this->data_.target,
                                                     0, TargetX, 0, TargetY)) {
                                MoveInfo move_info;
                                move_info.from_pos = from_pos;
                                move_info.move_to_pos = move_to_pos;
                                move_info.dir = cur_dir;
                                this->answer_.push_back(move_info);
                                foundLastStep = true;
                                break;
                            }
                            else {
                                std::swap(board.cells[from_pos], board.cells[move_to_pos]);
                            }
                        }
                    }

                    if (!foundLastStep) {
                        printf("translateMovePath():\n\n"
                               "The last step move is wrong.\n\n");
                    }
                }
            }
            else {
                printf("translateMovePath():\n\n"
                       "Move path have error, [move_to_pos] is not a empty gird,\n"
                       "index = %u, from_pos = (%u, %u), color = %u\n\n",
                       (uint32_t)(index + 1), (uint32_t)(move_to_pos / BoardY),
                       (uint32_t)(move_to_pos % BoardY), (uint32_t)move_to_cell);
                success = false;
            }
        }

        //printf("translateMovePath() end ...\n\n");
        return success;
    }

    void displayAnswer(const std::vector<MoveInfo> & answer) {
        size_t index = 0;
        printf("Answer_Move_Path[%u] = {\n", (uint32_t)answer.size());
        for (auto iter : answer) {
            size_t from_pos    = iter.from_pos.value;
            size_t move_to_pos = iter.move_to_pos.value;
            size_t dir         = iter.dir;
            printf("    [%2u]: from: (%u, %u), to: (%u, %u), dir: %-5s (%u)\n",
                   (uint32_t)(index + 1),
                   (uint32_t)(from_pos % BoardY), (uint32_t)(from_pos / BoardY),
                   (uint32_t)(move_to_pos % BoardY), (uint32_t)(move_to_pos / BoardY),
                   Direction::toString(dir),
                   (uint32_t)dir);
            index++;
        }
        printf("};\n\n");
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
                            this->data_.s456.depth_limit = std::min(size_t(35),
                                size_t(this->min_steps_ - stage_list[n].move_path.size()));
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
                                this->best_move_path_ = stage_list[n].move_path;
                                for (auto iter : this->move_path_) {
                                    this->best_move_path_.push_back(iter);
                                }
                                printf("Total moves: %u\n\n", (uint32_t)this->best_move_path_.size());
                            }
                        }
                    }
                }

                printf("min_steps: %u\n", (uint32_t)this->min_steps_);
                printf("Total moves: %u\n", (uint32_t)this->best_move_path_.size());
                printf("\n");

                if (this->min_steps_ != size_t(-1) || this->best_move_path_.size() > 0) {
                    solvable = true;
                    if (translateMovePath(this->best_move_path_)) {
                        displayAnswer(this->answer_);
                    }
                }
            }
        }

        return solvable;
    }

    bool solve_3x3() {
        SlidingPuzzle<TargetX, TargetY> slidingPuzzle;
        slidingPuzzle.template setPuzzle<BoardX, BoardY>(this->data_.board, this->data_.target);
        bool solvable = slidingPuzzle.solve();
        if (solvable) {
            translateMovePath(slidingPuzzle.getMovePath());
            this->map_used_ = slidingPuzzle.getMapUsed();
        }
        return solvable;
    }
};

} // namespace PuzzleGame
