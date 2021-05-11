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

#include "MagicBlock/AI/Constant.h"
#include "MagicBlock/AI/Color.h"
#include "MagicBlock/AI/Move.h"
#include "MagicBlock/AI/Board.h"
#include "MagicBlock/AI/Stage.h"
#include "MagicBlock/AI/ErrorCode.h"
#include "MagicBlock/AI/BitSet.h"

namespace MagicBlock {
namespace AI {

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t MaxValidValue = 8, std::size_t GridBits = 3,
          bool SearchAllAnswers = false>
class SlidingUnknownPuzzle
{
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

    typedef SlidingUnknownPuzzle<BoardX, BoardY> this_type;

    static const size_type BoardSize = BoardX * BoardY;
    static const size_type kSingelNumMaxCount = 4;

    static const size_type kMapBits = size_type(1U) << (BoardX * BoardY * GridBits);
    static const size_type kEmptyPosValue = MaxValidValue;
    static const size_type kUnknownPosValue = kEmptyPosValue + 1;
    static const size_type kMaxGridValue = kUnknownPosValue + 1;

    static const size_type MaxValidNumber = size_type(1U) << GridBits;
    static const size_type MaxNumber = (kMaxGridValue > MaxValidNumber) ? kMaxGridValue : MaxValidNumber;

    typedef Stage<BoardX, BoardY> stage_type;

private:
    Board<BoardX, BoardY> player_board_;
    Board<BoardX, BoardY> target_board_;

    std::vector<Board<BoardX, BoardY>> answer_list_;

    size_type map_used_;
    bool has_unknown_;

    int player_num_cnt_[MaxNumber];
    int target_num_cnt_[MaxNumber];

    std::vector<Move> empty_moves_[BoardSize];
    std::vector<Position> move_path_;

    void init() {
        for (size_type num = 0; num < MaxNumber; num++) {
            this->player_num_cnt_[num] = 0;
            this->target_num_cnt_[num] = 0;
        }

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                std::vector<Move> moves;
                for (size_type dir = Direction::First; dir < Direction::Last; dir++) {
                    assert(dir >= 0 && dir < 4);
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
    SlidingUnknownPuzzle() : map_used_(0), has_unknown_(false) {
        static_assert((MaxValidValue <= MaxNumber), "Error: MaxValidValue must less than or equal MaxNumber.");
        this->init();
    }

    ~SlidingUnknownPuzzle() {}

    size_type getMinSteps() const {
        return this->move_path_.size();
    }

    const std::vector<Position> & getMovePath() const {
        return this->move_path_;
    }

    size_type getMapUsed() const {
        return this->map_used_;
    }

    static uint8_t charToNumber(uint8_t ch) {
        if (ch >= '1' && ch <= '9') {
            uint8_t num = (ch - '1');
            return ((num < kEmptyPosValue) ? num : (uint8_t)-1);
        }
        else if (ch >= 'A' && ch <= 'Z') {
            uint8_t num = (ch - 'A');
            return ((num < kEmptyPosValue) ? num : (uint8_t)-1);
        }
        else if (ch == ' ' || ch == '0')
            return kEmptyPosValue;
        else if (ch == '?')
            return kUnknownPosValue;
        else
            return (uint8_t)-1;
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
                            uint8_t num = this_type::charToNumber(line[x]);
                            if (num >= 0 && num < kMaxGridValue) {
                                this->target_board_.cells[line_no * BoardY + x] = num;
                            }
                            else {
                                err_code = ErrorCode::TargetBoardNumberOverflow;
                                break;
                            }
                        }
                    }
                    else if (line_no >= (BoardY + 1) && line_no < (BoardY + 1 + BoardY)) {
                        size_type boardY = line_no - (BoardY + 1);
                        for (size_type x = 0; x < BoardX; x++) {
                            uint8_t num = this_type::charToNumber(line[x]);
                            if (num >= 0 && num < kMaxGridValue) {
                                if (num != kUnknownPosValue) {
                                    this->player_board_.cells[boardY * BoardY + x] = num;
                                }
                                else {
                                    // It can't be a unknown pos in player board
                                    err_code = ErrorCode::PlayerBoardNumberOverflow;
                                    break;
                                }
                            }
                            else {
                                err_code = ErrorCode::PlayerBoardNumberOverflow;
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
                         "SlidingUnknownPuzzle::readConfig() Error code: %d, reason: %s",
                         err_code, ErrorCode::toString(err_code));
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

    void count_all_num_cnt() {
        for (size_type num = 0; num < MaxNumber; num++) {
            this->player_num_cnt_[num] = 0;
            this->target_num_cnt_[num] = 0;
        }

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t num = this->player_board_.cells[y * BoardY + x];
                if (num >= 0 && num < kMaxGridValue) {
                    this->player_num_cnt_[num]++;
                }
            }
        }

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t num = this->target_board_.cells[y * BoardY + x];
                if (num >= 0 && num < kMaxGridValue) {
                    this->target_num_cnt_[num]++;
                }
            }
        }
    }

    int check_player_board_nums(size_type & duplicated_num) {
        int err_code = ErrorCode::Success;
        for (size_type num = 0; num < MaxNumber; num++) {
            if (this->player_num_cnt_[num] > (int)kSingelNumMaxCount && num != kUnknownPosValue) {
                err_code = ErrorCode::PlayerBoardNumberOverflow;
                duplicated_num = num;
                return err_code;
            }
        }
        return err_code;
    }

    int check_target_board_nums(size_type & duplicated_num) {
        int err_code = ErrorCode::Success;
        for (size_type num = 0; num < MaxNumber; num++) {
            if (this->target_num_cnt_[num] > 1 && num != kUnknownPosValue) {
                err_code = ErrorCode::TargetBoardNumberIsDuplicated;
                duplicated_num = num;
                return err_code;
            }
        }
        if (this->target_num_cnt_[kUnknownPosValue] == 0)
            this->has_unknown_ = false;
        else
            this->has_unknown_ = true;
        return err_code;
    }

    int check_all_board_nums(size_type & duplicated_num) {
        int err_code = this->check_player_board_nums(duplicated_num);
        if (ErrorCode::isSuccess(err_code)) {
            err_code = this->check_target_board_nums(duplicated_num);
        }
        return err_code;
    }

    int verify_board() {
        this->count_all_num_cnt();

        size_type duplicated_num;
        int err_code = this->check_all_board_nums(duplicated_num);
        if (ErrorCode::isFailure(err_code)) {
            char err_info[256] = {0};
            snprintf(err_info, sizeof(err_info) - 1,
                     "SlidingUnknownPuzzle::verify_board() Error code: %d, reason: %s\n"
                     "duplicated num: %d",
                     err_code, ErrorCode::toString(err_code), (int)duplicated_num);
            printf("%s\n\n", err_info);
        }

        return err_code;
    }

    template <size_type UBoardX, size_type UBoardY>
    void set_puzzle(const Board<UBoardX, UBoardY> & player,
                    const Board<BoardX, BoardY> & target,
                    size_type target_len) {
        static const ptrdiff_t startX = (UBoardX - BoardX) / 2;
        static const ptrdiff_t startY = (UBoardY - BoardY) / 2;
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                this->player_board_.cells[y * BoardY + x] =
                    player.cells[(startY + y) * UBoardY + (startX + x)];
            }
        }
        this->target_board_ = target;
    }

    bool find_empty(Position & empty_pos) const {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                char num = this->player_board_.cells[y * BoardY + x];
                if (num == kEmptyPosValue) {
                    empty_pos = (uint8_t)(y * BoardY + x);
                    return true;
                }
            }
        }
        return false;
    }

    bool is_satisfy(const Board<BoardX, BoardY> & currnet,
                    const Board<BoardX, BoardY> & target) const {
        if (!this->has_unknown_) {
            return (currnet == target);
        }
        else {
            for (size_type pos = 0; pos < BoardSize; pos++) {
                size_type target_num = target.cells[pos];
                bool is_unknown = (target_num == kUnknownPosValue);
                if (!is_unknown) {
                    size_type cur_num = currnet.cells[pos];
                    if (cur_num != target_num) {
                        return false;
                    }
                }
            }
            return true;
        }
    }

    bool solve() {
        if (this->is_satisfy(this->player_board_, this->target_board_)) {
            return true;
        }

        bool solvable = false;
        size_type depth = 0;

        Position empty;
        bool found_empty = this->find_empty(empty);
        if (found_empty) {
            jstd::BitSet<kMapBits> visited[BoardSize];

            stage_type start;
            start.empty_pos = empty;
            start.last_dir = uint8_t(-1);
            start.rotate_type = 0;
            start.board = this->player_board_;
            visited[empty.value].set(start.board.template compactValue<kEmptyPosValue>());

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size() > 0) {
                for (size_type i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    uint8_t empty_pos = stage.empty_pos;
                    const std::vector<Move> & empty_moves = this->empty_moves_[empty_pos];
                    size_type total_moves = empty_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = empty_moves[n].pos;
                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        size_type value64 = next_stage.board.template compactValue<kEmptyPosValue>();
                        if (visited[move_pos].test_and_set(value64))
                            continue;

                        //visited[move_pos].set(value64);
                        
                        next_stage.empty_pos = move_pos;
                        next_stage.last_dir = cur_dir;
                        //next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        next_stage.move_path.push_back(move_pos);

                        next_stages.push_back(next_stage);

                        if (this->is_satisfy(next_stage.board, this->target_board_)) {
                            this->move_path_ = next_stage.move_path;
                            assert((depth + 1) == next_stage.move_path.size());
                            this->answer_list_.push_back(next_stage.board);
                            solvable = true;
                            exit = true;
                            if (!SearchAllAnswers)
                                break;
                        }
                    }

                    if (!SearchAllAnswers) {
                        if (exit) {
                            break;
                        }
                    }
                }

                depth++;
                std::swap(cur_stages, next_stages);
                next_stages.clear();

                if (exit) {
                    break;
                }
            }

            size_type visited_count = 0;
            for (size_type i = 0; i < BoardSize; i++) {
                visited_count += visited[i].count();
            }
            this->map_used_ = visited_count;

            if (solvable) {
                Board<BoardX, BoardY>::template display_num_board<kEmptyPosValue, kUnknownPosValue>("Player Board", this->player_board_);
                Board<BoardX, BoardY>::template display_num_board<kEmptyPosValue, kUnknownPosValue>("Target Board", this->target_board_);
                if (SearchAllAnswers && this->answer_list_.size() > 1)
                    Board<BoardX, BoardY>::template display_num_boards<kEmptyPosValue, kUnknownPosValue>("Answer Board", this->answer_list_);
                else
                    Board<BoardX, BoardY>::template display_num_board<kEmptyPosValue, kUnknownPosValue>("Answer Board", this->answer_list_[0]);
            }
        }

        return solvable;
    }

    bool queue_solve() {
        if (this->is_satisfy(this->player_board_, this->target_board_)) {
            return true;
        }

        bool solvable = false;
        size_type depth = 0;

        Position empty;
        bool found_empty = this->find_empty(empty);
        if (found_empty) {
            jstd::BitSet<kMapBits> visited[BoardSize];

            stage_type start;
            start.empty_pos = empty;
            start.last_dir = uint8_t(-1);
            start.rotate_type = 0;
            start.board = this->player_board_;
            visited[empty.value].set(start.board.template compactValue<kEmptyPosValue>());

            std::queue<stage_type> cur_stages;
            std::queue<stage_type> next_stages;

            cur_stages.push(start);

            bool exit = false;
            while (cur_stages.size() > 0) {
                do {
                    const stage_type & stage = cur_stages.front();

                    uint8_t empty_pos = stage.empty_pos;
                    const std::vector<Move> & empty_moves = this->empty_moves_[empty_pos];
                    size_type total_moves = empty_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = empty_moves[n].pos;
                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        size_type value64 = next_stage.board.template compactValue<kEmptyPosValue>();
                        if (visited[move_pos].test_and_set(value64))
                            continue;

                        //visited[move_pos].set(value64);
                        
                        next_stage.empty_pos = move_pos;
                        next_stage.last_dir = cur_dir;
                        //next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        next_stage.move_path.push_back(move_pos);

                        next_stages.push(next_stage);

                        if (this->is_satisfy(next_stage.board, this->target_board_)) {
                            this->move_path_ = next_stage.move_path;
                            assert((depth + 1) == next_stage.move_path.size());
                            this->answer_list_.push_back(next_stage.board);
                            solvable = true;
                            exit = true;
                            if (!SearchAllAnswers)
                                break;
                        }
                    }

                    cur_stages.pop();

                    if (!SearchAllAnswers) {
                        if (exit) {
                            break;
                        }
                    }
                } while (cur_stages.size() > 0);

                depth++;
                std::swap(cur_stages, next_stages);

                if (exit) {
                    break;
                }
            }

            size_type visited_count = 0;
            for (size_type i = 0; i < BoardSize; i++) {
                visited_count += visited[i].count();
            }
            this->map_used_ = visited_count;

            if (solvable) {
                Board<BoardX, BoardY>::template display_num_board<kEmptyPosValue, kUnknownPosValue>("Player Board", this->player_board_);
                Board<BoardX, BoardY>::template display_num_board<kEmptyPosValue, kUnknownPosValue>("Target Board", this->target_board_);
                if (SearchAllAnswers && this->answer_list_.size() > 1)
                    Board<BoardX, BoardY>::template display_num_boards<kEmptyPosValue, kUnknownPosValue>("Answer Board", this->answer_list_);
                else
                    Board<BoardX, BoardY>::template display_num_board<kEmptyPosValue, kUnknownPosValue>("Answer Board", this->answer_list_[0]);
            }
        }

        return solvable;
    }
};

} // namespace AI
} // namespace MagicBlock
