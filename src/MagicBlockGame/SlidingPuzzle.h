#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include <vector>
#include <queue>
#include <algorithm>    // For std::swap(), until C++11
#include <utility>      // For std::swap(), since C++11

#include "Constant.h"
#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "Stage.h"
#include "BitSet.h"

namespace MagicBlock {

template <size_t BoardX, size_t BoardY>
class SlidingPuzzle
{
public:
    static const size_t kMapBits = 1ULL << (BoardX * BoardY * 3);

    typedef Stage<BoardX, BoardY> stage_type;

private:
    Board<BoardX, BoardY> player_board_;
    Board<BoardX, BoardY> target_board_[4];
    size_t target_len_;

    std::vector<Move> empty_moves_[BoardX * BoardY];
    std::vector<Position> move_path_;

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

    size_t getMapUsed() const {
        return this->map_used_;
    }

    template <size_t UBoardX, size_t UBoardY>
    void setPuzzle(const Board<UBoardX, UBoardY> & player,
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
            while (cur_stages.size()) {
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
                        Position next_move(stage.empty);
                        next_stage.move_path.push_back(next_move);

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
            while (!cur_stages.empty()) {
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
                        Position next_move(stage.empty);
                        next_stage.move_path.push_back(next_move);

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
                } while (!cur_stages.empty());

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
