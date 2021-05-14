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

#include "MagicBlock/AI/Constant.h"
#include "MagicBlock/AI/Color.h"
#include "MagicBlock/AI/Move.h"
#include "MagicBlock/AI/Board.h"
#include "MagicBlock/AI/SharedData.h"
#include "MagicBlock/AI/ErrorCode.h"
#include "MagicBlock/AI/Utils.h"
#include "MagicBlock/AI/StopWatch.h"

//
// Two endpoint / two leg algorithm
//
namespace MagicBlock {
namespace AI {
namespace internal {

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t TargetX, std::size_t TargetY,
          bool AllowRotate = true>
class BaseGame
{
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

    typedef BaseGame<BoardX, BoardY, TargetX, TargetY, AllowRotate> this_type;

    typedef SharedData<BoardX, BoardY, TargetX, TargetY>            shared_data_type;
    typedef typename shared_data_type::stage_type                   stage_type;
    typedef typename shared_data_type::player_board_t               player_board_t;
    typedef typename shared_data_type::target_board_t               target_board_t;

    typedef std::function<bool(size_type, size_type, const stage_type & stage)> phase2_callback;

    static const size_type BoardSize = BoardX * BoardY;
    static const size_type kSingelColorNums = (BoardSize - 1) / (Color::Last - 1);

    static const ptrdiff_t kStartX = (BoardX - TargetX) / 2;
    static const ptrdiff_t kStartY = (BoardY - TargetY) / 2;

protected:
    shared_data_type data_;

    size_type min_steps_;
    size_type map_used_;

    std::vector<Position> move_path_;
    std::vector<Position> best_move_path_;
    std::vector<MoveInfo> answer_;

    void assert_color(uint8_t color) const {
        assert(color >= Color::First && color < Color::Last);
    }

    // Initialize empty_moves[BoardSize]
    void init() {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                std::vector<Move> empty_moves;
                for (size_type dir = Direction::First; dir < Direction::Last; dir++) {
                    assert(dir >= 0 && dir < 4);
                    int board_x = (int)x + Dir_Offset[dir].x;
                    if (board_x < 0 || board_x >= (int)BoardX)
                        continue;
                    int board_y = (int)y + Dir_Offset[dir].y;
                    if (board_y < 0 || board_y >= (int)BoardY)
                        continue;
                    Move move;
                    move.pos = Position(board_y * (int)BoardX + board_x);
                    move.dir = (uint8_t)dir;
                    empty_moves.push_back(move);
                }
                assert((y * BoardX + x) < BoardSize);
                this->data_.empty_moves[y * BoardX + x] = empty_moves;
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

    ~BaseGame() {
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

    int readConfig(const char * filename) {
        int err_code = ErrorCode::Success;
        size_type line_no = 0;
        std::ifstream ifs;
        try {
            ifs.open(filename, std::ios::in);
            if (!ifs.fail()) {
                while (!ifs.eof()) {
                    char line[256];
                    std::fill_n(line, sizeof(line), 0);
                    ifs.getline(line, 256);
                    if (line_no >= 0 && line_no < TargetY) {
                        for (size_type x = 0; x < TargetX; x++) {
                            uint8_t color = Color::charToColor(line[x]);
                            if (color >= Color::First && color < Color::Last) {
                                this->data_.target_board[0].cells[line_no * TargetX + x] = color;
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
                            if (color >= Color::First && color < Color::Last) {
                                this->data_.player_board.cells[boardY * BoardX + x] = color;
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
                }

                ifs.close();
            }
            else {
                std::ios::iostate rdstate = ifs.rdstate();
                if ((rdstate & std::ifstream::failbit) != 0) {
                    err_code = ErrorCode::ifstream_IsFailed;
                }
                else if ((rdstate & std::ifstream::badbit) != 0) {
                    err_code = ErrorCode::ifstream_IsBad;
                }
            }

            if (ErrorCode::isFailure(err_code)) {
                char err_info[256] = {0};
                snprintf(err_info, sizeof(err_info) - 1,
                         "MagicBlockBaseGame::readConfig() Error code: %d, reason: %s",
                         err_code, ErrorCode::toString(err_code));
                //throw std::runtime_error(err_info);
                printf("%s\n\n", err_info);
            }
        }
        catch (std::exception & ex) {
            err_code = ErrorCode::StdException;
            std::cout << "Exception: " << ex.what() << std::endl << std::endl;
        }

        if (ErrorCode::isSuccess(err_code)) {
            err_code = this->verify_board();
        }

        return err_code;
    }

    int verify_board() {
        int err_code = ErrorCode::Success;

        this->count_all_color_nums();

        int result = this->check_all_board_colors();
        if (ErrorCode::isSuccess(result)) {
            this->rotate_target_board();
        }
        else {
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
                         "BaseGame::verify_board() Error code: %d, reason: %s",
                         err_code, ErrorCode::toString(err_code));
                printf("%s\n\n", err_info);
            }
        }
        
        return err_code;
    }

    static void display_target_board(const char * title, const Board<TargetX, TargetY> & board) {
        Board<TargetX, TargetY>::display_board(title, board);
    }

    static void display_player_board(const char * title, const Board<BoardX, BoardY> & board) {
        Board<BoardX, BoardY>::display_board(title, board);
    }

    void count_all_color_nums() {
        for (size_type clr = Color::First; clr < Color::Maximum; clr++) {
            this->data_.player_colors[clr] = 0;
            this->data_.target_colors[clr] = 0;
        }

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t clr = this->data_.player_board.cells[y * BoardX + x];
                assert_color(clr);
                this->data_.player_colors[clr]++;
            }
        }

        for (size_type y = 0; y < TargetY; y++) {
            for (size_type x = 0; x < TargetX; x++) {
                uint8_t clr = this->data_.target_board[0].cells[y * TargetX + x];
                assert_color(clr);
                this->data_.target_colors[clr]++;
            }
        }
    }

    int check_player_board_colors() {
        int err_code = ErrorCode::Success;
        for (size_type clr = Color::First; clr < Color::Maximum; clr++) {
            if (this->data_.player_colors[clr] > (int)kSingelColorNums) {
                err_code = ErrorCode::PlayerBoardColorOverflowFirst + (int)clr;
                return err_code;
            }
        }
        return err_code;
    }

    int check_target_board_colors() {
        int err_code = ErrorCode::Success;
        for (size_type clr = Color::First; clr < Color::Maximum; clr++) {
            if (this->data_.target_colors[clr] > (int)kSingelColorNums) {
                err_code = ErrorCode::TargetBoardColorOverflowFirst + (int)clr;
                return err_code;
            }
        }
        return err_code;
    }

    int check_all_board_colors() {
        int err_code = this->check_player_board_colors();
        if (ErrorCode::isSuccess(err_code)) {
            err_code = this->check_target_board_colors();
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

        if (AllowRotate)
            this->data_.target_len = target_len;
        else
            this->data_.target_len = 1;
    }

    bool find_empty(const Board<BoardX, BoardY> & board, Position & empty_pos) const {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t color = board.cells[y * BoardX + x];
                if (color == Color::Empty) {
                    empty_pos = (uint8_t)(y * BoardX + x);
                    return true;
                }
            }
        }
        return false;
    }

    bool is_satisfy(const Board<BoardX, BoardY> & player,
                    const Board<TargetX, TargetY> & target) const {
        for (size_type y = 0; y < TargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetX;
            ptrdiff_t playerBaseY = (kStartY + y) * BoardX;
            for (size_type x = 0; x < TargetX; x++) {
                uint8_t target_clr = target.cells[targetBaseY + x];
                uint8_t player_clr = player.cells[playerBaseY + (kStartX + x)];
                assert_color(target_clr);
                assert_color(player_clr);
                if (player_clr != target_clr) {
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
            ptrdiff_t targetBaseY = y * TargetX;
            ptrdiff_t playerBaseY = (kStartY + y) * BoardX;
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
                uint8_t target_clr = target.cells[targetBaseY + x];
                uint8_t player_clr = board.cells[baseY + (kStartX + x)];
                assert_color(target_clr);
                assert_color(player_clr);
                if (player_clr != target_clr) {
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

    bool translateMovePath(const std::vector<Position> & move_path, std::vector<MoveInfo> & answer) const {
        return this->data_.player_board.translate_move_path(move_path, answer);
    }

    bool translateMovePath(const std::vector<Position> & move_path) {
        return this->translateMovePath(move_path, this->answer_);
    }

    bool translateMovePath(const stage_type & target_stage) {
        return this->translateMovePath(target_stage.move_path);
    }

    bool translateMovePath(const stage_type & target_stage, std::vector<MoveInfo> & answer) {
        return this->translateMovePath(target_stage.move_path, answer);
    }

    void displayAnswer(const std::vector<MoveInfo> & answer) const {
        player_board_t::template display_answer<BoardX>(answer);
    }

    void displayAnswer() const {
        this->displayAnswer(this->answer_);
    }

    void displayAnswer(const std::vector<Position> & move_path) const {
        std::vector<MoveInfo> answer;
        if (this->translateMovePath(move_path, answer)) {
            this->displayAnswer(answer);
        }
    }

    void displayAnswer(const stage_type & target_stage) {
        std::vector<MoveInfo> answer;
        if (this->translateMovePath(target_stage, answer)) {
            this->displayAnswer(answer);
        }
    }
};

} // namespace internal
} // namespace AI
} // namespace MagicBlock
