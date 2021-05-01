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

#include "Constant.h"
#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "SharedData.h"
#include "ErrorCode.h"
#include "v1/Solver.h"
#include "SlidingPuzzle.h"
#include "Utils.h"
#include "StopWatch.h"

namespace MagicBlock {
namespace v1 {

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t TargetX, std::size_t TargetY,
          bool AllowRotate = true>
class Game
{
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

    static const size_type kSingelColorNums = (BoardX * BoardY - 1) / (Color::Last - 1);

    static const ptrdiff_t startX = (BoardX - TargetX) / 2;
    static const ptrdiff_t startY = (BoardY - TargetY) / 2;

    typedef Game<BoardX, BoardY, TargetX, TargetY, AllowRotate>                     this_type;
    typedef typename SharedData<BoardX, BoardY, TargetX, TargetY>::stage_type       stage_type;

    typedef std::function<bool(std::size_t, std::size_t, const stage_type & stage)> phase2_callback;

    typedef Solver<BoardX, BoardY, TargetX, TargetY, AllowRotate, PhaseType::Phase1_123, phase2_callback>  Phase1Solver;
    typedef Solver<BoardX, BoardY, TargetX, TargetY, false,       PhaseType::Phase2,     phase2_callback>  Phase2Solver;

private:
    SharedData<BoardX, BoardY, TargetX, TargetY> data_;

    size_type min_steps_;
    size_type map_used_;

    std::vector<Position> move_path_;
    std::vector<Position> best_move_path_;
    std::vector<MoveInfo> answer_;

    void assert_color(uint8_t color) const {
        assert(color >= Color::Empty && color < Color::Last);
    }

    void init() {
        // Initialize empty_moves[BoardX * BoardY]
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                std::vector<Move> empty_moves;
                for (size_type dir = Direction::First; dir < Direction::Last; dir++) {
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

        if (AllowRotate)
            this->data_.target_len = MAX_ROTATE_TYPE;
        else
            this->data_.target_len = 1;
    }

public:
    Game() : min_steps_(std::numeric_limits<ssize_type>::max()), map_used_(0) {
        this->init();
    }

    ~Game() {}

    size_type getMinSteps() const {
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

    size_type getMapUsed() const {
        return this->map_used_;
    }

    int readInput(const char * filename) {
        int err_code = ErrorCode::Success;
        size_type line_no = 0;
        std::ifstream ifs;
        try {
            ifs.open(filename, std::ios::in);
            if (ifs.good()) {
                err_code = 0;
                do {
                    char line[256];
                    std::fill_n(line, sizeof(line), 0);
                    ifs.getline(line, 256);
                    if (line_no >= 0 && line_no < TargetY) {
                        for (size_type x = 0; x < TargetX; x++) {
                            uint8_t color = Color::charToColor(line[x]);
                            if (color >= Color::Empty && color < Color::Last) {
                                this->data_.target_board[0].cells[line_no * TargetY + x] = color;
                            }
                            else {
                                err_code = ErrorCode::UnknownTargetBoardColor;
                                break;
                            }
                        }
                    }
                    else if (line_no >= (TargetY + 1) && line_no < (TargetY + 1 + BoardY)) {
                        size_type boardY = line_no - (TargetY + 1);
                        for (size_type x = 0; x < BoardX; x++) {
                            uint8_t color = Color::charToColor(line[x]);
                            if (color >= Color::Empty && color < Color::Last) {
                                this->data_.player_board.cells[boardY * BoardY + x] = color;
                            }
                            else {
                                err_code = ErrorCode::UnknownPlayerBoardColor;
                                break;
                            }
                        }
                    }
                    if (ErrorCode::isFailure(err_code))
                        break;
                    line_no++;
                } while (!ifs.eof());

                ifs.close();

                if (ErrorCode::isFailure(err_code)) {
                    char err_info[256] = {0};
                    snprintf(err_info, sizeof(err_info) - 1,
                             "MagicBlockGame::readInput() Error code: %d, reason: %s",
                             err_code, ErrorCode::toString(err_code));
                    throw std::runtime_error(err_info);
                }
            }
        }
        catch (std::exception & ex) {
            if (ErrorCode::isSuccess(err_code)) {
                err_code = ErrorCode::StdException;
            }
            std::cout << "Exception: " << ex.what() << std::endl << std::endl;
        }

        if (ErrorCode::isSuccess(err_code)) {
            count_color_nums();

            int result = check_board_colors();
            if (ErrorCode::isFailure(result)) {
                if (result >= ErrorCode::TargetBoardColorOverflowFirst &&
                    result <= ErrorCode::TargetBoardColorOverflowLast) {
                    err_code = ErrorCode::TargetBoardColorOverflow;
                }
                else if (result >= ErrorCode::PlayerBoardColorOverflowFirst &&
                         result <= ErrorCode::PlayerBoardColorOverflowLast) {
                    err_code = ErrorCode::PlayerBoardColorOverflow;
                }

                if (ErrorCode::isFailure(err_code)) {
                    char err_info[256] = {0};
                    snprintf(err_info, sizeof(err_info) - 1,
                             "MagicBlockGame::readInput() Error code: %d, reason: %s",
                             err_code, ErrorCode::toString(err_code));
                }
            }

            rotate_target_board();
        }

        return err_code;
    }

    void count_color_nums() {
        for (size_type clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->data_.player_colors[clr] = 0;
            this->data_.target_colors[clr] = 0;
        }

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t cell = this->data_.player_board.cells[y * BoardY + x];
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->data_.player_colors[cell]++;
                }
            }
        }

        for (size_type y = 0; y < TargetY; y++) {
            for (size_type x = 0; x < TargetX; x++) {
                uint8_t cell = this->data_.target_board[0].cells[y * TargetY + x];
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->data_.target_colors[cell]++;
                }
            }
        }
    }

    int check_player_board_colors() {
        int err_code = ErrorCode::Success;
        for (size_type clr = 0; clr < Color::Maximum; clr++) {
            if (this->data_.player_colors[clr] > (int)kSingelColorNums) {
                err_code = ErrorCode::PlayerBoardColorOverflowFirst + (int)clr;
                return err_code;
            }
        }
        return err_code;
    }

    int check_target_board_colors() {
        int err_code = ErrorCode::Success;
        for (size_type clr = 0; clr < Color::Maximum; clr++) {
            if (this->data_.target_colors[clr] > (int)kSingelColorNums) {
                err_code = ErrorCode::TargetBoardColorOverflowFirst + (int)clr;
                return err_code;
            }
        }
        return err_code;
    }

    int check_board_colors() {
        int err_code = check_player_board_colors();
        if (ErrorCode::isSuccess(err_code)) {
            err_code = check_target_board_colors();
        }
        return err_code;
    }

    void rotate_target_board() {
        this->data_.target_board[0].rotate_90_cw(this->data_.target_board[1]);
        this->data_.target_board[0].rotate_180_cw(this->data_.target_board[2]);
        this->data_.target_board[0].rotate_270_cw(this->data_.target_board[3]);

        bool is_duplicated[4];
        for (size_type i = 0; i < 4; i++) {
            is_duplicated[i] = false;
        }

        for (size_type i = 1; i < 4; i++) {
            for (size_type j = 0; j < i; j++) {
                if (this->data_.target_board[j] == this->data_.target_board[i]) {
                    is_duplicated[i] = true;
                    break;
                }
            }
        }

        size_type target_len = 4;
        for (size_type i = 1; i < 4; i++) {
            if (is_duplicated[i]) {
                for (size_type j = i + 1; j < 4; j++) {
                    this->data_.target_board[j - 1] = this->data_.target_board[j];
                }
                target_len--;
            }
        }
        assert(target_len > 0);

        this->data_.target_len = target_len;
    }

    bool find_empty(const Board<BoardX, BoardY> & board, Position & empty_pos) const {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t cell = board.cells[y * BoardY + x];
                if (cell == Color::Empty) {
                    empty_pos = (uint8_t)(y * BoardY + x);
                    return true;
                }
            }
        }
        return false;
    }

    bool is_satisfy(const Board<BoardX, BoardY> & player,
                    const Board<TargetX, TargetY> & target) const {
        for (size_type y = 0; y < TargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_type x = 0; x < TargetX; x++) {
                uint8_t target_cell = target.cells[targetBaseY + x];
                uint8_t cell = player.cells[baseY + (startX + x)];
                if (cell != target_cell) {
                    return false;
                }
            }
        }

        return true;
    }

    size_type is_satisfy(const Board<BoardX, BoardY> & player,
                         const Board<TargetX, TargetY> target[4],
                         size_type target_len) const {
        for (size_type index = 0; index < target_len; index++) {
            if (is_satisfy(player, target[index])) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    // Check order: up to down
    bool check_board_is_equal(const Board<BoardX, BoardY> & board,
                              const Board<TargetX, TargetY> & target,
                              size_type firstTargetX, size_type lastTargetX,
                              size_type firstTargetY, size_type lastTargetY) {
        for (size_type y = firstTargetY; y < lastTargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
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

    // Check order: up to down
    size_type check_board_is_equal(const Board<BoardX, BoardY> & board,
                                   const Board<TargetX, TargetY> target[4],
                                   size_type target_len,
                                   size_type firstTargetX, size_type lastTargetX,
                                   size_type firstTargetY, size_type lastTargetY) {
        for (size_type index = 0; index < target_len; index++) {
            if (check_board_is_equal(board, target[index],
                firstTargetX, lastTargetX, firstTargetY, lastTargetY)) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    bool translateMovePath(const std::vector<Position> & move_path) {
        bool success = true;

        //printf("translateMovePath() begin ...\n\n");
        this->answer_.clear();

        Board<BoardX, BoardY> board(this->data_.player_board);
        size_type index = 0;
        uint8_t from_pos, move_pos;
        uint8_t last_dir = uint8_t(-1);
        if (move_path.size() > 0) {
            index++;
            move_pos = move_path[0].value;
            uint8_t move_to_cell = board.cells[move_pos];
            assert_color(move_to_cell);
            if (move_to_cell == Color::Empty) {
                for (size_type i = 1; i < move_path.size(); i++) {
                    from_pos = move_path[i].value;
                    uint8_t from_cell = board.cells[from_pos];
                    assert_color(from_cell);
                    if (from_cell != Color::Empty) {
                        last_dir = Direction::template getDir<BoardX, BoardY>(from_pos, move_pos);
                        MoveInfo move_info;
                        move_info.from_pos = from_pos;
                        move_info.move_pos = move_pos;
                        move_info.color = board.cells[from_pos];
                        move_info.dir = last_dir;
                        this->answer_.push_back(move_info);

                        std::swap(board.cells[from_pos], board.cells[move_pos]);
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
                    move_pos = from_pos;
                    index++;
                }

                if (success) {
                    bool foundLastStep = false;
                    move_to_cell = board.cells[move_pos];
                    assert_color(move_to_cell);
                    if (move_to_cell == Color::Empty) {
                        const std::vector<Move> & empty_moves = this->data_.empty_moves[move_pos];
                        size_type total_moves = empty_moves.size();
                        for (size_type n = 0; n < total_moves; n++) {
                            uint8_t dir = empty_moves[n].dir;
                            from_pos = empty_moves[n].pos;
                            uint8_t cur_dir = Direction::template getDir<BoardX, BoardY>(from_pos, move_pos);
                            assert(dir != cur_dir);
                            if (cur_dir == last_dir)
                                continue;

                            std::swap(board.cells[from_pos], board.cells[move_pos]);

                            if (check_board_is_equal(board, this->data_.target_board, this->data_.target_len,
                                                     0, TargetX, 0, TargetY)) {
                                MoveInfo move_info;
                                move_info.from_pos = from_pos;
                                move_info.move_pos = move_pos;
                                move_info.color = board.cells[move_pos];
                                move_info.dir = cur_dir;
                                this->answer_.push_back(move_info);
                                foundLastStep = true;
                                break;
                            }
                            else {
                                std::swap(board.cells[from_pos], board.cells[move_pos]);
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
                       "Move path have error, [move_pos] is not a empty gird,\n"
                       "index = %u, from_pos = (%u, %u), color = %u\n\n",
                       (uint32_t)(index + 1), (uint32_t)(move_pos / BoardY),
                       (uint32_t)(move_pos % BoardY), (uint32_t)move_to_cell);
                success = false;
            }
        }

        //printf("translateMovePath() end ...\n\n");
        return success;
    }

    const char posToChr(size_type pos) {
        return (char)('A' + (uint8_t)(pos % 256));
    }

    void displayAnswer(const std::vector<MoveInfo> & answer) {
        size_type index = 0;
        printf("Answer_Move_Path[%u] = {\n", (uint32_t)answer.size());
        for (auto iter : answer) {
            size_type from_pos    = iter.from_pos.value;
            size_type move_pos = iter.move_pos.value;
            size_type color       = iter.color;
            size_type dir         = iter.dir;
            printf("    [%2u]: [%s], %c%u --> %c%u, dir: %-5s (%u)\n",
                   (uint32_t)(index + 1),
                   Color::toShortString(color),
                   (uint32_t)posToChr(from_pos / BoardY), (uint32_t)(from_pos % BoardY) + 1,
                   (uint32_t)posToChr(move_pos / BoardY), (uint32_t)(move_pos % BoardY) + 1,
                   Direction::toString(dir),
                   (uint32_t)dir);
            index++;
        }
        printf("};\n\n");
    }

    bool solve() {
        if (is_satisfy(this->data_.player_board, this->data_.target_board, this->data_.target_len) != 0) {
            return true;
        }

        bool solvable = false;
        size_type out_rotate_type = 0;

        Position empty;
        bool found_empty = find_empty(this->data_.player_board, empty);
        if (found_empty) {
            jtest::StopWatch sw;

            Phase1Solver solver(&this->data_);
            sw.start();
            bool phase1_solvable = solver.solve(out_rotate_type);
            sw.stop();

            if (phase1_solvable) {
                double elapsed_time = sw.getElapsedMillisec();
                printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
                System::pause();

                for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                    for (size_type phase1_type = 0; phase1_type < MAX_PHASE1_TYPE; phase1_type++) {
                        const std::vector<stage_type> & stage_list = this->data_.phase1.stage_list[rotate_type][phase1_type];
                        size_type totalStage = stage_list.size();
                        for (size_type n = 0; n < totalStage; n++) {
                            size_type move_path_size = stage_list[n].move_path.size();
                            if (this->min_steps_ > move_path_size) {
                                this->data_.phase2.depth_limit = std::min(ssize_type(MAX_PHASE2_DEPTH),
                                    std::max(ssize_type(this->min_steps_ - 1 - move_path_size), ssize_type(1)));
                            }
                            else {
                                continue;
                            }

                            this->data_.phase2.rotate_type = rotate_type;
                            this->data_.phase2.phase1_type = phase1_type;
                            this->data_.phase2.index = n;

                            Phase2Solver phase2_solver(&this->data_);
                            phase2_solver.setPlayerBoard(stage_list[n].board);
                            phase2_solver.setRotateType(stage_list[n].rotate_type);

                            bool phase2_solvable = phase2_solver.solve(out_rotate_type);
                            if (phase2_solvable) {
                                solvable = true;
                                this->move_path_ = phase2_solver.getMovePath();
                                size_type total_steps = move_path_size + this->move_path_.size();
                                printf("Phase1 moves: %u, Phase2 moves: %u, Total moves: %u\n\n",
                                       (uint32_t)move_path_size,
                                       (uint32_t)this->move_path_.size(),
                                       (uint32_t)total_steps);

                                if (total_steps < this->min_steps_) {
                                    this->map_used_ = phase2_solver.getMapUsed();
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

    bool bitset_phase2_search(size_type rotate_type, size_type phase1_type, const stage_type & stage) {
        static size_type phase2_stage_cnt = 0;
        this->data_.phase2.index = phase2_stage_cnt;
        phase2_stage_cnt++;

        size_type move_path_size = stage.move_path.size();
        if (this->min_steps_ > move_path_size) {
            this->data_.phase2.depth_limit = std::min(ssize_type(MAX_PHASE2_DEPTH),
                std::max(ssize_type(this->min_steps_ - 1 - move_path_size), ssize_type(1)));
        }
        else {
            return false;
        }

        this->data_.phase2.rotate_type = rotate_type;
        this->data_.phase2.phase1_type = phase1_type;

        Phase2Solver solver(&this->data_);
        solver.setPlayerBoard(stage.board);
        solver.setRotateType(rotate_type);

        size_type out_rotate_type = 0;
        phase2_callback dummy_phase2_search;

        bool solvable = solver.bitset_solve(out_rotate_type, dummy_phase2_search);
        if (solvable) {
            this->move_path_ = solver.getMovePath();
            size_type total_steps = stage.move_path.size() + this->move_path_.size();
            printf("Phase1 moves: %u, Phase2 moves: %u, Total moves: %u\n\n",
                    (uint32_t)stage.move_path.size(),
                    (uint32_t)this->move_path_.size(),
                    (uint32_t)total_steps);

            if (total_steps < this->min_steps_) {
                this->map_used_ = solver.getMapUsed();
                this->min_steps_ = total_steps;
                this->best_move_path_ = stage.move_path;
                for (auto iter : this->move_path_) {
                    this->best_move_path_.push_back(iter);
                }
                printf("Total moves: %u\n\n", (uint32_t)this->best_move_path_.size());
            }
        }

        return solvable;
    }

    bool bitset_solve() {
        if (is_satisfy(this->data_.player_board, this->data_.target_board, this->data_.target_len) != 0) {
            return true;
        }

        bool solvable = false;
        size_type out_rotate_type = 0;

        using namespace std::placeholders;
        phase2_callback phase_search_cb = std::bind(&Game::bitset_phase2_search, this, _1, _2, _3);

        Position empty;
        bool found_empty = find_empty(this->data_.player_board, empty);
        if (found_empty) {
            jtest::StopWatch sw;

            Phase1Solver solver(&this->data_);
            sw.start();
            solvable = solver.bitset_solve(out_rotate_type, phase_search_cb);
            sw.stop();

            if (solvable) {
                double elapsed_time = sw.getElapsedMillisec();
                printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);

                printf("min_steps: %u\n", (uint32_t)this->min_steps_);
                printf("Total moves: %u\n", (uint32_t)this->best_move_path_.size());
                printf("\n");

                if (this->min_steps_ != size_type(-1) || this->best_move_path_.size() > 0) {
                    solvable = true;
                    if (translateMovePath(this->best_move_path_)) {
                        displayAnswer(this->answer_);
                    }
                }
            }
        }

        return solvable;
    }

    bool stand_alone_bitset_solve() {
        if (is_satisfy(this->data_.player_board, this->data_.target_board, this->data_.target_len) != 0) {
            return true;
        }

        bool solvable = false;
        size_type out_rotate_type = 0;
        phase2_callback phase_search_cb;

        Position empty;
        bool found_empty = find_empty(this->data_.player_board, empty);
        if (found_empty) {
            jtest::StopWatch sw;

            Phase1Solver solver(&this->data_);
            sw.start();
            bool phase1_solvable = solver.bitset_solve(out_rotate_type, phase_search_cb);
            sw.stop();

            if (phase1_solvable) {
                double elapsed_time = sw.getElapsedMillisec();
                printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
                //System::pause();

                for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                    for (size_type phase1_type = 0; phase1_type < MAX_PHASE1_TYPE; phase1_type++) {
                        const std::vector<stage_type> & stage_list = this->data_.phase1.stage_list[rotate_type][phase1_type];
                        size_type totalStage = stage_list.size();
                        for (size_type n = 0; n < totalStage; n++) {
                            size_type move_path_size = stage_list[n].move_path.size();
                            if (this->min_steps_ > move_path_size) {
                                this->data_.phase2.depth_limit = std::min(ssize_type(MAX_PHASE2_DEPTH),
                                    std::max(ssize_type(this->min_steps_ - 1 - move_path_size), ssize_type(5)));
                            }
                            else {
                                continue;
                            }

                            this->data_.phase2.rotate_type = rotate_type;
                            this->data_.phase2.phase1_type = phase1_type;
                            this->data_.phase2.index = n;

                            Phase2Solver phase2_solver(&this->data_);
                            phase2_solver.setPlayerBoard(stage_list[n].board);
                            phase2_solver.setRotateType(stage_list[n].rotate_type);

                            bool phase2_solvable = phase2_solver.bitset_solve(out_rotate_type, phase_search_cb);
                            if (phase2_solvable) {
                                solvable = true;
                                this->move_path_ = phase2_solver.getMovePath();
                                size_type total_steps = move_path_size + this->move_path_.size();
                                printf("Phase1 moves: %u, Phase2 moves: %u, Total moves: %u\n\n",
                                       (uint32_t)move_path_size,
                                       (uint32_t)this->move_path_.size(),
                                       (uint32_t)total_steps);

                                if (total_steps < this->min_steps_) {
                                    this->map_used_ = phase2_solver.getMapUsed();
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
                }

                printf("min_steps: %u\n", (uint32_t)this->min_steps_);
                printf("Total moves: %u\n", (uint32_t)this->best_move_path_.size());
                printf("\n");

                if (this->min_steps_ != size_type(-1) || this->best_move_path_.size() > 0) {
                    solvable = true;
                    if (translateMovePath(this->best_move_path_)) {
                        displayAnswer(this->answer_);
                    }
                }
            }
        }

        return solvable;
    }

    bool solve_sliding_puzzle() {
        SlidingPuzzle<TargetX, TargetY> slidingPuzzle;
        slidingPuzzle.template setPuzzle<BoardX, BoardY>(this->data_.player_board,
                                                         this->data_.target_board,
                                                         this->data_.target_len);
        bool solvable = slidingPuzzle.solve();
        if (solvable) {
            this->best_move_path_ = slidingPuzzle.getMovePath();
            this->map_used_ = slidingPuzzle.getMapUsed();
        }
        return solvable;
    }

    bool queue_solve_sliding_puzzle() {
        SlidingPuzzle<TargetX, TargetY> slidingPuzzle;
        slidingPuzzle.template setPuzzle<BoardX, BoardY>(this->data_.player_board,
                                                         this->data_.target_board,
                                                         this->data_.target_len);
        bool solvable = slidingPuzzle.queue_solve();
        if (solvable) {
            this->best_move_path_ = slidingPuzzle.getMovePath();
            this->map_used_ = slidingPuzzle.getMapUsed();
        }
        return solvable;
    }
};

} // namespace v1
} // namespace MagicBlock
