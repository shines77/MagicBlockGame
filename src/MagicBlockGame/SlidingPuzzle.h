#pragma once

#include <stdint.h>
#include <stddef.h>

#include <vector>
#include <algorithm>    // For std::swap(), until C++11
#include <utility>      // For std::swap(), since C++11

#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "Stage.h"
#include "BitSet.h"

namespace PuzzleGame {

template <size_t BoardX, size_t BoardY>
class SlidingPuzzle
{
public:
    static const size_t kMapBits = 1ULL << (BoardX * BoardY * 3);

    typedef Stage<BoardX, BoardY> stage_type;

private:
    Board<BoardX, BoardY> board_;
    Board<BoardX, BoardY> target_;

    jstd::BitSet<kMapBits> visited_;

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
                    move.pos = Position(board_y * BoardY + board_x);
                    move.dir = (uint8_t)dir;
                    moves.push_back(move);
                }
                this->empty_moves_[y * BoardY + x] = moves;
            }
        }
    }

public:
    SlidingPuzzle() : map_used_(0) {
        this->init();
    }
    ~SlidingPuzzle() {}

    size_t getSteps() const {
        return this->move_path_.size();
    }

    const std::vector<Position> & getMovePath() const {
        return this->move_path_;
    }

    size_t getMapUsed() const {
        return this->map_used_;
    }

    template <size_t UBoardX, size_t UBoardY>
    void setPuzzle(const Board<UBoardX, UBoardY> & board, const Board<BoardX, BoardY> & target) {
        static const ptrdiff_t startX = (UBoardX - BoardX) / 2;
        static const ptrdiff_t startY = (UBoardY - BoardY) / 2;
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                this->board_.cells[y * BoardY + x] = board.cells[(startY + y) * UBoardY + (startX + x)];
            }
        }
        this->target_ = target;
    }

    bool find_empty(Position & empty_pos) const {
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

    bool solve() {
        if (this->board_ == this->target_) {
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position empty;
        bool found_empty = find_empty(empty);
        if (found_empty) {
            stage_type start;
            start.empty = empty;
            start.last_dir = -1;
            start.board = this->board_;
            this->visited_.set(start.board.value());

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size()) {
                for (size_t i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    int empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->empty_moves_[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        int16_t move_pos = empty_moves[n].pos.value;
                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        size_t board_value = next_stage.board.value();
                        if (this->visited_.test(board_value))
                            continue;

                        this->visited_.set(board_value);
                        
                        next_stage.empty.value = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.move_path = stage.move_path;
                        Position next_move(stage.empty);
                        next_stage.move_path.push_back(next_move);

                        next_stages.push_back(next_stage);

                        if (next_stage.board == this->target_) {
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
                this->map_used_ = visited_.count();
            }
        }

        return solvable;
    }
};

} // namespace PuzzleGame
