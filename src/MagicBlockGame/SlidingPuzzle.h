#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>    // For std::swap(), until C++11
#include <utility>      // For std::swap(), since C++11

#include "Constant.h"
#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "Stage.h"
#include "ErrorCode.h"
#include "BitSet.h"

namespace MagicBlock {

template <std::size_t BoardX, std::size_t BoardY, bool AllowRotate = true>
class SlidingPuzzle
{
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

    static const size_type kMapBits = size_type(1U) << (BoardX * BoardY * 3);
    static const size_type kSingelColorNums = 4;

    typedef Stage<BoardX, BoardY> stage_type;

private:
    Board<BoardX, BoardY> player_board_;
    Board<BoardX, BoardY> target_board_[4];

    size_type target_len_;
    size_type map_used_;

    int player_colors_[Color::Maximum];
    int target_colors_[Color::Maximum];

    std::vector<Move> empty_moves_[BoardX * BoardY];
    std::vector<Position> move_path_;

    void init() {
        for (size_type clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->player_colors_[clr] = Color::Empty;
            this->target_colors_[clr] = Color::Empty;
        }

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                std::vector<Move> moves;
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
                    moves.push_back(move);
                }
                this->empty_moves_[y * BoardY + x] = moves;
            }
        }
    }

public:
    SlidingPuzzle() : target_len_(0), map_used_(0) {
        this->init();
    }

    ~SlidingPuzzle() {}

    size_t getMinSteps() const {
        return this->move_path_.size();
    }

    const std::vector<Position> & getMovePath() const {
        return this->move_path_;
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
                    if (line_no >= 0 && line_no < BoardY) {
                        for (size_type x = 0; x < BoardX; x++) {
                            uint8_t color = Color::charToColor(line[x]);
                            if (color >= Color::Empty && color < Color::Last) {
                                this->target_board_[0].cells[line_no * BoardY + x] = color;
                            }
                            else {
                                err_code = ErrorCode::UnknownTargetBoardColor;
                                break;
                            }
                        }
                    }
                    else if (line_no >= (BoardY + 1) && line_no < (BoardY + 1 + BoardY)) {
                        size_type boardY = line_no - (BoardY + 1);
                        for (size_type x = 0; x < BoardX; x++) {
                            uint8_t color = Color::charToColor(line[x]);
                            if (color >= Color::Empty && color < Color::Last) {
                                this->player_board_.cells[boardY * BoardY + x] = color;
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
                         "SlidingPuzzle::readConfig() Error code: %d, reason: %s",
                         err_code, ErrorCode::toString(err_code));
                throw std::runtime_error(err_info);
            }
        }
        catch (std::exception & ex) {
            if (ErrorCode::isSuccess(err_code)) {
                err_code = ErrorCode::StdException;
            }
            std::cout << "Exception: " << ex.what() << std::endl << std::endl;
        }

        if (ErrorCode::isSuccess(err_code)) {
            err_code = this->verify_board_validity();
        }

        return err_code;
    }

    void count_all_color_nums() {
        for (size_type clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->player_colors_[clr] = 0;
            this->target_colors_[clr] = 0;
        }

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t cell = this->player_board_.cells[y * BoardY + x];
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->player_colors_[cell]++;
                }
            }
        }

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t cell = this->target_board_[0].cells[y * BoardY + x];
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->target_colors_[cell]++;
                }
            }
        }
    }

    int check_player_board_colors() {
        int err_code = ErrorCode::Success;
        for (size_type clr = 0; clr < Color::Maximum; clr++) {
            if (this->player_colors_[clr] > (int)kSingelColorNums) {
                err_code = ErrorCode::PlayerBoardColorOverflowFirst + (int)clr;
                return err_code;
            }
        }
        return err_code;
    }

    int check_target_board_colors() {
        int err_code = ErrorCode::Success;
        for (size_type clr = 0; clr < Color::Maximum; clr++) {
            if (this->target_colors_[clr] > (int)kSingelColorNums) {
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
        this->target_board_[0].rotate_90_cw(this->target_board_[1]);
        this->target_board_[0].rotate_180_cw(this->target_board_[2]);
        this->target_board_[0].rotate_270_cw(this->target_board_[3]);

        bool is_duplicated[4];
        for (size_type i = 0; i < 4; i++) {
            is_duplicated[i] = false;
        }

        for (size_type i = 1; i < 4; i++) {
            for (size_type j = 0; j < i; j++) {
                if (this->target_board_[j] == this->target_board_[i]) {
                    is_duplicated[i] = true;
                    break;
                }
            }
        }

        size_type target_len = 4;
        for (size_type i = 1; i < 4; i++) {
            if (is_duplicated[i]) {
                for (size_type j = i + 1; j < 4; j++) {
                    this->target_board_[j - 1] = this->target_board_[j];
                }
                target_len--;
            }
        }
        assert(target_len > 0);

        this->target_len_ = target_len;
    }

    int verify_board_validity() {
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
                         "SlidingPuzzle::verify_board_validity() Error code: %d, reason: %s",
                         err_code, ErrorCode::toString(err_code));
            }
        }

        return err_code;
    }

    template <size_t UBoardX, size_t UBoardY>
    void set_puzzle(const Board<UBoardX, UBoardY> & player,
                    const Board<BoardX, BoardY> target[4],
                    size_t target_len) {
        static const ptrdiff_t startX = (UBoardX - BoardX) / 2;
        static const ptrdiff_t startY = (UBoardY - BoardY) / 2;
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                this->player_board_.cells[y * BoardY + x] = player.cells[(startY + y) * UBoardY + (startX + x)];
            }
        }
        for (size_t i = 0; i < 4; i++) {
            this->target_board_[i] = target[i];
        }
        this->target_len_ = target_len;
    }

    bool find_empty(Position & empty_pos) const {
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                char color = this->player_board_.cells[y * BoardY + x];
                if (color == Color::Empty) {
                    empty_pos = (uint8_t)(y * BoardY + x);
                    return true;
                }
            }
        }
        return false;
    }

    bool is_satisfy(const Board<BoardX, BoardY> & player,
                    const Board<BoardX, BoardY> & target) const {
        return (player == target);
    }

    size_t is_satisfy(const Board<BoardX, BoardY> & player,
                      const Board<BoardX, BoardY> target[4],
                      size_t target_len) const {
        for (size_t index = 0; index < target_len; index++) {
            if (player == target[index]) {
                return index;
            }
        }

        return size_t(-1);
    }

    bool solve() {
        if (is_satisfy(this->player_board_, this->target_board_, this->target_len_) != size_t(-1)) {
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position empty;
        bool found_empty = find_empty(empty);
        if (found_empty) {
            jstd::BitSet<kMapBits> visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = uint8_t(-1);
            start.board = this->player_board_;
            visited.set(start.board.value());

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size() > 0) {
                for (size_t i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    uint8_t empty_pos = stage.empty;
                    const std::vector<Move> & empty_moves = this->empty_moves_[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = empty_moves[n].pos;
                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        size_t value64 = next_stage.board.value();
                        if (visited.test(value64))
                            continue;

                        visited.set(value64);
                        
                        next_stage.empty = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.move_path = stage.move_path;
                        next_stage.move_path.push_back(move_pos);

                        next_stages.push_back(next_stage);

                        if (is_satisfy(next_stage.board, this->target_board_, this->target_len_) != size_t(-1)) {
                            this->move_path_ = next_stage.move_path;
                            assert((depth + 1) == next_stage.move_path.size());
                            solvable = true;
                            exit = true;
                            break;
                        }
                    }

                    if (exit) {
                        break;
                    }
                }

                depth++;
                std::swap(cur_stages, next_stages);
                next_stages.clear();

                if (exit) {
                    break;
                }
            }

            if (solvable) {
                this->map_used_ = visited.count();
            }
        }

        return solvable;
    }

    bool queue_solve() {
        if (is_satisfy(this->player_board_, this->target_board_, this->target_len_) != size_t(-1)) {
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position empty;
        bool found_empty = find_empty(empty);
        if (found_empty) {
            jstd::BitSet<kMapBits> visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = uint8_t(-1);
            start.board = this->player_board_;
            visited.set(start.board.value());

            std::queue<stage_type> cur_stages;
            std::queue<stage_type> next_stages;

            cur_stages.push(start);

            bool exit = false;
            while (cur_stages.size() > 0) {
                do {
                    const stage_type & stage = cur_stages.front();

                    uint8_t empty_pos = stage.empty;
                    const std::vector<Move> & empty_moves = this->empty_moves_[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = empty_moves[n].pos;
                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        size_t value64 = next_stage.board.value();
                        if (visited.test(value64))
                            continue;

                        visited.set(value64);
                        
                        next_stage.empty = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.move_path = stage.move_path;
                        next_stage.move_path.push_back(move_pos);

                        next_stages.push(next_stage);

                        if (is_satisfy(next_stage.board, this->target_board_, this->target_len_) != size_t(-1)) {
                            this->move_path_ = next_stage.move_path;
                            assert((depth + 1) == next_stage.move_path.size());
                            solvable = true;
                            exit = true;
                            break;
                        }
                    }

                    cur_stages.pop();

                    if (exit) {
                        break;
                    }
                } while (cur_stages.size() > 0);

                depth++;
                std::swap(cur_stages, next_stages);

                if (exit) {
                    break;
                }
            }

            if (solvable) {
                this->map_used_ = visited.count();
            }
        }

        return solvable;
    }
};

} // namespace MagicBlock
