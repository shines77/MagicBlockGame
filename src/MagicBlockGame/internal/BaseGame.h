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
#include "Utils.h"
#include "StopWatch.h"

//
// Two endpoint / two leg algorithm
//
namespace MagicBlock {
namespace internal {

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t TargetX, std::size_t TargetY,
          bool AllowRotate = true>
class BaseGame
{
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

    static const size_type kSingelColorNums = (BoardX * BoardY - 1) / (Color::Last - 1);

    static const ptrdiff_t kStartX = (BoardX - TargetX) / 2;
    static const ptrdiff_t kStartY = (BoardY - TargetY) / 2;

    typedef BaseGame<BoardX, BoardY, TargetX, TargetY, AllowRotate> this_type;
    typedef SharedData<BoardX, BoardY, TargetX, TargetY>            shared_data_type;
    typedef typename shared_data_type::stage_type                   stage_type;

    typedef std::function<bool(size_type, size_type, const stage_type & stage)> phase2_callback;

protected:
    shared_data_type data_;

    size_type min_steps_;
    size_type map_used_;

    int player_colors[Color::Maximum];
    int target_colors[Color::Maximum];

    std::vector<Position> move_path_;
    std::vector<Position> best_move_path_;
    std::vector<MoveInfo> answer_;

    void assert_color(uint8_t color) const {
        assert(color >= Color::Empty && color < Color::Last);
    }

    // Initialize empty_moves[BoardX * BoardY]
    void init() {
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
    BaseGame() : min_steps_(std::numeric_limits<ssize_type>::max()), map_used_(0) {
        this->init();
    }

    virtual ~BaseGame() {
        this->destory();
    }

    void destory() {
        // TODO:
    }

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
                             "MagicBlockBaseGame::readInput() Error code: %d, reason: %s",
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
            this->count_all_color_nums();

            int result = this->check_all_board_colors();
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
                             "MagicBlockBaseGame::readInput() Error code: %d, reason: %s",
                             err_code, ErrorCode::toString(err_code));
                }
            }

            this->rotate_target_board();
        }

        return err_code;
    }

    void count_all_color_nums() {
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

    int check_all_board_colors() {
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
            ptrdiff_t baseY = (kStartY + y) * BoardY;
            for (size_type x = 0; x < TargetX; x++) {
                uint8_t target_cell = target.cells[targetBaseY + x];
                uint8_t cell = player.cells[baseY + (kStartX + x)];
                assert_color(target_cell);
                assert_color(cell);
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
            if (this->is_satisfy(player, target[index])) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    // Check order: up to down
    bool partial_target_is_satisfy(const Board<BoardX, BoardY> & board,
                                   const Board<TargetX, TargetY> & target,
                                   size_type firstTargetX, size_type lastTargetX,
                                   size_type firstTargetY, size_type lastTargetY) {
        for (size_type y = firstTargetY; y < lastTargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (kStartY + y) * BoardY;
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
                uint8_t target_cell = target.cells[targetBaseY + x];
                uint8_t cell = board.cells[baseY + (kStartX + x)];
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
    size_type partial_target_is_satisfy(const Board<BoardX, BoardY> & board,
                                        const Board<TargetX, TargetY> target[4],
                                        size_type target_len,
                                        size_type firstTargetX, size_type lastTargetX,
                                        size_type firstTargetY, size_type lastTargetY) {
        for (size_type index = 0; index < target_len; index++) {
            if (this->partial_target_is_satisfy(board, target[index],
                                                firstTargetX, lastTargetX,
                                                firstTargetY, lastTargetY)) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    const char posToChr(size_type pos) const {
        return (char)('A' + (uint8_t)(pos % 256));
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
            uint8_t move_cell = board.cells[move_pos];
            assert_color(move_cell);
            if (move_cell == Color::Empty) {
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
                    move_cell = board.cells[move_pos];
                    assert_color(move_cell);
                    if (move_cell == Color::Empty) {
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

                            if (this->partial_target_is_satisfy(board,
                                                                this->data_.target_board,
                                                                this->data_.target_len,
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
                       (uint32_t)(move_pos % BoardY), (uint32_t)move_cell);
                success = false;
            }
        }

        //printf("translateMovePath() end ...\n\n");
        return success;
    }

    void displayAnswer(const std::vector<MoveInfo> & answer) {
        size_type index = 0;
        printf("Answer_Move_Path[%u] = {\n", (uint32_t)answer.size());
        for (auto iter : answer) {
            size_type from_pos  = iter.from_pos.value;
            size_type move_pos  = iter.move_pos.value;
            size_type color     = iter.color;
            size_type dir       = iter.dir;
            printf("    [%2u]: [%s], %c%u --> %c%u, dir: %-5s (%u)\n",
                   (uint32_t)(index + 1),
                   Color::colorToChar(color),
                   (uint32_t)posToChr(from_pos / BoardY), (uint32_t)(from_pos % BoardY) + 1,
                   (uint32_t)posToChr(move_pos / BoardY), (uint32_t)(move_pos % BoardY) + 1,
                   Direction::toString(dir),
                   (uint32_t)dir);
            index++;
        }
        printf("};\n\n");
    }
};

} // namespace internal
} // namespace MagicBlock
