#pragma once

#include <stdint.h>
#include <stddef.h>

#include <vector>
#include <algorithm>    // For std::swap(), until C++11
#include <utility>      // For std::swap(), since C++11

#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "BitSet.h"

namespace PuzzleGame {

template <size_t BoardX, size_t BoardY>
class SlidingPuzzle
{
public:
    static const size_t kBitmapBits = 1ULL << 27;

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

private:
    Board<BoardX, BoardY> board_;
    Board<BoardX, BoardY> target_;

    std::vector<State> cur_;
    std::vector<State> next_;

    jstd::BitSet<kBitmapBits> visited_;

    std::vector<Move> empty_moves_[BoardX * BoardY];
    std::vector<Move> moves_;

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
                    move.pos = Position16(board_y * BoardY + board_x);
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
        return this->moves_.size();
    }

    const std::vector<Move> & getMoves() const {
        return this->moves_;
    }

    size_t getMapUsed() const {
        return this->map_used_;
    }

    template <size_t BlockX, size_t BlockY>
    void setPuzzle(Board<BlockX, BlockY> blocks, Board<BoardX, BoardY> target) {
        ptrdiff_t startX = (BlockX - BoardX) / 2;
        ptrdiff_t startY = (BlockY - BoardY) / 2;
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                this->board_.cells[y * BoardY + x] = blocks.cells[(startY + y) * BlockY + (startX + x)];
                this->target_.cells[y * BoardY + x] = target.cells[y * BoardY + x];
            }
        }
    }

    bool find_empty(Position16 & empty_pos) const {
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

        Position16 empty;
        bool found_empty = find_empty(empty);
        if (found_empty) {
            State first;
            first.empty = empty;
            first.last_dir = -1;
            first.board = this->board_;
            this->visited_.set(first.board.value());

            this->cur_.push_back(first);

            bool found = false;
            while (this->cur_.size()) {
                for (size_t i = 0; i < this->cur_.size(); i++) {
                    const State & state = this->cur_[i];

                    int empty_pos = state.empty.value;
                    const std::vector<Move> & empty_moves = this->empty_moves_[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        if (empty_moves[n].dir == state.last_dir)
                            continue;

                        State next_state(state.board);
                        int move_pos = empty_moves[n].pos.value;
                        std::swap(next_state.board.cells[empty_pos], next_state.board.cells[move_pos]);
                        size_t board_value = next_state.board.value();
                        if (this->visited_.test(board_value))
                            continue;

                        this->visited_.set(board_value);
                        
                        Position16 next_empty(move_pos);
                        next_state.empty = next_empty;
                        next_state.last_dir = (uint8_t)n;
                        next_state.moves = state.moves;
                        Move next_move;
                        next_move.pos = state.empty;
                        next_move.dir = (uint8_t)n;
                        next_state.moves.push_back(next_move);

                        if (next_state.board == this->target_) {
                            this->moves_ = next_state.moves;
                            assert((depth + 1) == next_state.moves.size());
                            found = true;
                            break;
                        }

                        this->next_.push_back(next_state);
                    }
                }

                if (found) {
                    break;
                }

                depth++;
                std::swap(this->cur_, this->next_);
                this->next_.clear();
            }

            if (found) {
                solvable = true;
                this->map_used_ = visited_.count();
            }
        }

        return solvable;
    }
};

} // namespace PuzzleGame
