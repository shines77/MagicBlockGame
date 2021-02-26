#pragma once

#include "Color.h"
#include "Move.h"
#include "Board.h"
#include "BitSet.h"

template <size_t BoardX, size_t BoardY>
class SlidingPuzzle
{
public:
    static const size_t kBitmapBits = 1ULL << 27;

    struct State {
        Position empty;
        uint8_t last_dir, reserve;
        Board<BoardX, BoardY> board;
        std::vector<Move> moves;
    };

private:
    Board<BoardX, BoardY> board_;
    Board<BoardX, BoardY> target_;

    std::vector<State> cur_;
    std::vector<State> next_;

    jstd::BitSet<kBitmapBits> visited_;

    std::vector<Move> empty_moves_[BoardX * BoardY];
    std::vector<Move> moves_;

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
                    move.pos = Position(board_x, board_y);
                    move.dir = (uint8_t)dir;
                    moves.push_back(move);
                }
                this->empty_moves_[y * BoardY + x] = moves;
            }
        }
    }

public:
    SlidingPuzzle() {
        this->init();
    }
    ~SlidingPuzzle() {}

    size_t getSteps() const {
        return this->moves_.size();
    }

    const std::vector<Move> & getMoves() const {
        return this->moves_;
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

    bool find_empty(Position & empty_pos) const {
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                char color = this->board_.cells[y * BoardY + x];
                if (color == Color::Empty) {
                    empty_pos.x = (uint8_t)x;
                    empty_pos.y = (uint8_t)y;
                    return true;
                }
            }
        }
        return false;
    }

    bool solve() {
        bool solvable = false;

        size_t min_steps = 0;
        size_t steps = 1;

        // Find empty position
        Position empty;
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
                    if (state.board == this->target_) {
                        min_steps = steps;
                        found = true;
                        break;
                    }

                    empty = state.empty;
                    int empty_pos = empty.y * BoardY + empty.x;
                    size_t max_idx = this->empty_moves_[empty_pos].size();
                    for (size_t idx = 0; idx < max_idx; idx++) {
                        if (this->empty_moves_[empty_pos][idx].dir == state.last_dir)
                            continue;
                        //int grid_x = empty.x + Dir_Offset[dir].x;
                        int grid_x = this->empty_moves_[empty_pos][idx].pos.x;
                        assert(grid_x >= 0 && grid_x < (int)BoardX);
                        //int grid_y = empty.y + Dir_Offset[dir].y;
                        int grid_y = this->empty_moves_[empty_pos][idx].pos.y;
                        assert(grid_y >= 0 && grid_y < (int)BoardX);
                        int grid_pos = grid_y * BoardY + grid_x;

                        Board<BoardX, BoardY> board = state.board;
                        std::swap(board.cells[empty_pos], board.cells[grid_pos]);
                        size_t board_value = board.value();
                        if (this->visited_.test(board_value))
                            continue;

                        this->visited_.set(board_value);

                        State next_state;
                        Position next_empty(grid_x, grid_y);
                        next_state.empty = next_empty;
                        next_state.last_dir = (uint8_t)idx;
                        next_state.board = board;
                        next_state.moves = state.moves;
                        Move next_move;
                        next_state.moves.push_back(next_move);
                        this->next_.push_back(next_state);
                    }
                }

                if (found) {
                    break;
                }

                steps++;
                std::swap(this->cur_, this->next_);
                this->next_.clear();
            }

            if (found) {
                solvable = true;
            }
        }

        return solvable;
    }
};
