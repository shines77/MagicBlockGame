#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <queue>
#include <set>
#include <exception>
#include <stdexcept>
#include <algorithm>    // For std::swap(), until C++11. std::min()
#include <utility>      // For std::swap(), since C++11

#include "Constant.h"
#include "Color.h"
#include "Move.h"
#include "Value128.h"
#include "Board.h"
#include "Stage.h"
#include "SharedData.h"
#include "SparseBitset.h"

//#define ENABLE_DEBUG

namespace MagicBlock {

template <size_t BoardX, size_t BoardY,
          size_t TargetX, size_t TargetY,
          bool AllowRotate, size_t Step>
class Solver
{
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

#ifdef NDEBUG
    static const size_t kMinSearchDepth = 15;
    static const size_t kMaxSearchDepth = 27;

    static const size_t kDefaultSearchDepthLimit = 30;

    static const size_t kSlideDepth = 6;
    static const size_t kMaxSlideDepth = 10;
#else
    static const size_t kMinSearchDepth = 15;
    static const size_t kMaxSearchDepth = 21;

    static const size_t kDefaultSearchDepthLimit = 22;

    static const size_t kSlideDepth = 1;
    static const size_t kMaxSlideDepth = 2;
#endif
    static const size_t kSingelColorNums = (BoardX * BoardY - 1) / (Color::Last - 1);

    static const ptrdiff_t startX = (BoardX - TargetX) / 2;
    static const ptrdiff_t startY = (BoardY - TargetY) / 2;

    typedef typename SharedData<BoardX, BoardY, TargetX, TargetY>::stage_type stage_type;

private:
    SharedData<BoardX, BoardY, TargetX, TargetY> * data_;

    Board<BoardX, BoardY> player_board_;
    Board<TargetX, TargetY> target_board_[4];

    size_t target_len_;
    size_t rotate_type_;

    int partial_colors_[Color::Maximum];

    std::vector<Position> move_path_;

    size_t map_used_;

    void assert_color(uint8_t color) const {
        assert(color >= Color::Empty && color < Color::Last);
    }

    void count_target_color_nums(const Board<TargetX, TargetY> & target) {
        for (size_t clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->data_->target_colors[clr] = 0;
        }

        for (size_t y = 0; y < TargetY; y++) {
            for (size_t x = 0; x < TargetX; x++) {
                uint8_t cell = target.cells[y * TargetY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->data_->target_colors[cell]++;
                }
            }
        }
    }

    void count_partial_target_color_nums(const Board<TargetX, TargetY> & target,
                                         size_t firstTargetX, size_t lastTargetX,
                                         size_t firstTargetY, size_t lastTargetY) {
        for (size_t clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->data_->target_colors[clr] = 0;
        }

        for (size_t y = firstTargetY; y < lastTargetY; y++) {
            for (size_t x = firstTargetX; x < lastTargetX; x++) {
                uint8_t cell = target.cells[y * TargetY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->data_->target_colors[cell]++;
                }
            }
        }
    }

    void locked_partial_board(int locked[BoardX * BoardY],
                              size_t firstX, size_t lastX,
                              size_t firstY, size_t lastY) {
        for (size_t y = 0; y < BoardY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_t x = 0; x < BoardX; x++) {
                locked[baseY + x] = 0;
            }
        }

        for (size_t y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_t x = firstX; x < lastX; x++) {
                locked[baseY + x] = 1;
            }
        }
    }

    void init() {
        if (Step == 1 || Step == 12 || Step == 123) {
            this->player_board_ = this->data_->player_board;
            for (size_t i = 0; i < MaxRotateType; i++) {
                this->target_board_[i] = this->data_->target_board[i];
            }
            if (AllowRotate)
                this->target_len_ = this->data_->target_len;
            else
                this->target_len_ = 1;

            count_target_color_nums(this->target_board_[0]);

            data_->s123.init(kDefaultSearchDepthLimit);

            // Reset lock_inited[]
            for (size_t phrase1_type = 0; phrase1_type < MaxPhrase1Type; phrase1_type++) {
                this->data_->s456.lock_inited[phrase1_type] = 0;
            }
        }
        else if (Step == 456) {
            //
        }
    }

    void init_target_board_locked(size_t rotate_type) {
        assert(rotate_type >= 0 && rotate_type < MaxRotateType);
        this->target_board_[0] = this->data_->target_board[rotate_type];
        this->target_len_ = 1;

        size_t phrase1_type = this->data_->s456.phrase1_type;
        if (this->data_->s456.lock_inited[phrase1_type] == 0) {
            this->data_->s456.lock_inited[phrase1_type] = 1;
            switch (phrase1_type) {
                case 0:
                    // Top partial
                    count_partial_target_color_nums(this->target_board_[0], 0, TargetX, TargetY - 1, TargetY);
                    locked_partial_board(this->data_->s456.locked, 0, BoardX, 0, startY + 1);
                    break;
                case 1:
                    // Left partial
                    count_partial_target_color_nums(this->target_board_[0], TargetX - 1, TargetX, 0, TargetY);
                    locked_partial_board(this->data_->s456.locked, 0, startX + 1, 0, BoardY);
                    break;
                case 2:
                    // Right partial
                    count_partial_target_color_nums(this->target_board_[0], 0, 1, 0, TargetY);
                    locked_partial_board(this->data_->s456.locked, startX + TargetX - 1, BoardX, 0, BoardY);
                    break;
                case 3:
                    // Bottom partial
                    count_partial_target_color_nums(this->target_board_[0], 0, TargetX, 0, 1);
                    locked_partial_board(this->data_->s456.locked, 0, BoardX, startY + TargetY - 1, BoardY);
                    break;
                default:
                    assert(false);
                    break;
            }
        }
    }

public:
    Solver(SharedData<BoardX, BoardY, TargetX, TargetY> * data)
        : data_(data), target_len_(0), rotate_type_(0), map_used_(0) {
        this->init();
    }

    ~Solver() {}

    size_t getMinSteps() const {
        return this->move_path_.size();
    }

    const std::vector<Position> & getMovePath() const {
        return this->move_path_;
    }

    size_t getMapUsed() const {
        return this->map_used_;
    }

    void setPlayerBoard(const Board<BoardX, BoardY> & board) {
        this->player_board_ = board;
    }

    void setRotateType(size_t rotate_type) {
        assert(rotate_type >= 0 && rotate_type < MaxRotateType);
        this->rotate_type_ = rotate_type;
        init_target_board_locked(rotate_type);
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

    void count_reverse_partial_color_nums(const Board<BoardX, BoardY> & board,
                                          size_t firstX, size_t lastX,
                                          size_t firstY, size_t lastY) {
#ifdef ENABLE_DEBUG
        Board<BoardX, BoardY> test_board;
        for (size_t y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_t x = firstX; x < lastX; x++) {
                test_board.cells[baseY + x] = 1;
            }
        }
#endif
        this->partial_colors_[Color::Empty] = 0;
        for (size_t clr = Color::First; clr < Color::Last; clr++) {
            this->partial_colors_[clr] = kSingelColorNums;
        }

        for (size_t y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_t x = firstX; x < lastX; x++) {
                uint8_t cell = board.cells[baseY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Last) {
                    this->partial_colors_[cell]--;
                }
            }
        }
    }

    void count_partial_color_nums(const Board<BoardX, BoardY> & board,
                                  size_t firstX, size_t lastX,
                                  size_t firstY, size_t lastY) {
        for (size_t clr = Color::Empty; clr < Color::Last; clr++) {
            this->partial_colors_[clr] = 0;
        }

        for (size_t y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_t x = firstX; x < lastX; x++) {
                uint8_t cell = board.cells[baseY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Last) {
                    this->partial_colors_[cell]++;
                }
            }
        }
    }

    bool check_partial_color_nums() const {
        if (Step == 456) {
            for (size_t clr = Color::Empty; clr < Color::Last; clr++) {
                if (this->partial_colors_[clr] < this->data_->target_colors[clr]) {
                    return false;
                }
            }
        }
        else {
            for (size_t clr = Color::First; clr < Color::Last; clr++) {
                if (this->partial_colors_[clr] < this->data_->target_colors[clr]) {
                    return false;
                }
            }
        }
        return true;
    }

    bool is_satisfy_full(const Board<BoardX, BoardY> & player,
                         const Board<TargetX, TargetY> & target) const {
        for (size_t y = 0; y < TargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_t x = 0; x < TargetX; x++) {
                uint8_t target_cell = target.cells[targetBaseY + x];
                uint8_t cell = player.cells[baseY + (startX + x)];
                assert_color(target_cell);
                assert_color(cell);
                if (cell != target_cell) {
                    return false;
                }
            }
        }

        return true;
    }

    size_t is_satisfy_full(const Board<BoardX, BoardY> & player,
                           const Board<TargetX, TargetY> target[4],
                           size_t target_len) const {
        for (size_t index = 0; index < target_len; index++) {
            if (is_satisfy_full(player, target[index])) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    size_t is_satisfy_step_1(const Board<BoardX, BoardY> & player,
                             const Board<TargetX, TargetY> & target) {
        static const ptrdiff_t startX = (BoardX - TargetX) / 2;
        static const ptrdiff_t startY = (BoardY - TargetY) / 2;

        size_t mask = 0;

        // Left-Top Corner
        static const ptrdiff_t LeftTopX = startX;
        static const ptrdiff_t LeftTopY = startY;

        if (player.cells[LeftTopY * BoardY + LeftTopX] ==
            target.cells[0 * TargetY + 0]) {
            count_reverse_partial_color_nums(player, 0, LeftTopX + 1, 0, LeftTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 1;
            }
        }

        // Right-Top Corner
        static const ptrdiff_t RightTopX = startX + TargetX - 1;
        static const ptrdiff_t RightTopY = startY;

        if (player.cells[RightTopY * BoardY + RightTopX] ==
            target.cells[0 * TargetY + (TargetX - 1)]) {
            count_reverse_partial_color_nums(player, RightTopX, BoardX, 0, RightTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 2;
            }
        }

        // Left-Bottom Corner
        static const ptrdiff_t LeftBottomX = startX;
        static const ptrdiff_t LeftBottomY = startY + TargetY - 1;

        if (player.cells[LeftBottomY * BoardY + LeftBottomX] ==
            target.cells[(TargetY - 1) * TargetY + 0]) {
            count_reverse_partial_color_nums(player, 0, LeftBottomX + 1, LeftBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 4;
            }
        }

        // Right-Bottom Corner
        static const ptrdiff_t RightBottomX = startX + TargetX - 1;
        static const ptrdiff_t RightBottomY = startY + TargetY - 1;

        if (player.cells[RightBottomY * BoardY + RightBottomX] ==
            target.cells[(TargetY - 1) * TargetY + (TargetX - 1)]) {
            count_reverse_partial_color_nums(player, RightBottomX, BoardX, RightBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 8;
            }
        }

        size_u result(mask, 0);
        return result.value;
    }

    size_t is_satisfy_step_1(const Board<BoardX, BoardY> & player,
                             const Board<TargetX, TargetY> target[4],
                             size_t target_len) {
        for (size_t index = 0; index < target_len; index++) {
            size_t mask = is_satisfy_step_1(player, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    // Check order: up to down
    bool check_board_is_equal(const Board<BoardX, BoardY> & player,
                              const Board<TargetX, TargetY> & target,
                              size_t firstTargetX, size_t lastTargetX,
                              size_t firstTargetY, size_t lastTargetY) {
#ifdef ENABLE_DEBUG
        Board<BoardX, BoardY> test_board;
        for (size_t y = firstTargetY; y < lastTargetY; y++) {
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_t x = firstTargetX; x < lastTargetX; x++) {
                test_board.cells[baseY + (startX + x)] = 1;
            }
        }
#endif
        for (size_t y = firstTargetY; y < lastTargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_t x = firstTargetX; x < lastTargetX; x++) {
                uint8_t target_cell = target.cells[targetBaseY + x];
                uint8_t cell = player.cells[baseY + (startX + x)];
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
    size_t check_board_is_equal(const Board<BoardX, BoardY> & player,
                                const Board<TargetX, TargetY> target[4],
                                size_t target_len,
                                size_t firstTargetX, size_t lastTargetX,
                                size_t firstTargetY, size_t lastTargetY) {
        for (size_t index = 0; index < target_len; index++) {
            if (check_board_is_equal(player, target[index],
                firstTargetX, lastTargetX, firstTargetY, lastTargetY)) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    // Check order: down to up
    bool reverse_check_board_is_equal(const Board<BoardX, BoardY> & player,
                                      const Board<TargetX, TargetY> & target,
                                      size_t firstTargetX, size_t lastTargetX,
                                      size_t firstTargetY, size_t lastTargetY) {
        for (ptrdiff_t y = ptrdiff_t(lastTargetY - 1); y >= ptrdiff_t(firstTargetX); y--) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_t x = firstTargetX; x < lastTargetX; x++) {
                uint8_t target_cell = target.cells[targetBaseY + x];
                uint8_t cell = player.cells[baseY + (startX + x)];
                assert_color(target_cell);
                assert_color(cell);
                if (cell != target_cell) {
                    return false;
                }
            }
        }
        return true;
    }

    // Check order: down to up
    size_t reverse_check_board_is_equal(const Board<BoardX, BoardY> & player,
                                        const Board<TargetX, TargetY> & target,
                                        size_t target_len,
                                        size_t firstTargetX, size_t lastTargetX,
                                        size_t firstTargetY, size_t lastTargetY) {
        for (size_t index = 0; index < target_len; index++) {
            if (reverse_check_board_is_equal(player, target[index],
                firstTargetX, lastTargetX, firstTargetY, lastTargetY)) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    size_t is_satisfy_step_12(const Board<BoardX, BoardY> & player,
                              const Board<TargetX, TargetY> & target) {
        size_t mask = 0;

        // Left-Top Corner
        static const ptrdiff_t LeftTopX = startX;
        static const ptrdiff_t LeftTopY = startY;

        if (check_board_is_equal(player, target, 0, 2, 0, 1)) {
            count_reverse_partial_color_nums(player, 0, LeftTopX + 2, 0, LeftTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 1;
            }
        }

        // Right-Top Corner
        static const ptrdiff_t RightTopX = startX + TargetX - 1;
        static const ptrdiff_t RightTopY = startY;

        if (check_board_is_equal(player, target, TargetX - 2, TargetX, 0, 1)) {
            count_reverse_partial_color_nums(player, RightTopX - 1, BoardX, 0, RightTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 2;
            }
        }

        // Left-Bottom Corner
        static const ptrdiff_t LeftBottomX = startX;
        static const ptrdiff_t LeftBottomY = startY + TargetY - 1;

        if (check_board_is_equal(player, target, 0, 2, TargetY - 1, TargetY)) {
            count_reverse_partial_color_nums(player, 0, LeftBottomX + 2, LeftBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 4;
            }
        }

        // Right-Bottom Corner
        static const ptrdiff_t RightBottomX = startX + TargetX - 1;
        static const ptrdiff_t RightBottomY = startY + TargetY - 1;

        if (check_board_is_equal(player, target, TargetX - 2, TargetX, TargetY - 1, TargetY)) {
            count_reverse_partial_color_nums(player, RightBottomX - 1, BoardX, RightBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 8;
            }
        }

        size_u result(mask, 0);
        return result.value;
    }

    size_t is_satisfy_step_12(const Board<BoardX, BoardY> & player,
                              const Board<TargetX, TargetY> target[4],
                              size_t target_len) {
        for (size_t index = 0; index < target_len; index++) {
            size_t mask = is_satisfy_step_12(player, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    size_t is_satisfy_step_123(const Board<BoardX, BoardY> & player,
                               const Board<TargetX, TargetY> & target) {
        size_t mask = 0;

        // Top partial
        static const ptrdiff_t TopY = startY;

        if (check_board_is_equal(player, target, 0, TargetX, 0, 1)) {
            count_reverse_partial_color_nums(player, 0, BoardX, 0, TopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 1;
            }
        }

        // Left partial
        static const ptrdiff_t LeftX = startX;

        if (check_board_is_equal(player, target, 0, 1, 0, TargetY)) {
            count_reverse_partial_color_nums(player, 0, LeftX + 1, 0, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 2;
            }
        }

        // Right partial
        static const ptrdiff_t RightX = startX + TargetX - 1;

        if (check_board_is_equal(player, target, TargetX - 1, TargetX, 0, TargetY)) {
            count_reverse_partial_color_nums(player, RightX, BoardX, 0, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 4;
            }
        }

        // Bottom partial
        static const ptrdiff_t BottomY = startY + TargetY - 1;

        if (check_board_is_equal(player, target, 0, TargetX, TargetY - 1, TargetY)) {
            count_reverse_partial_color_nums(player, 0, BoardX, BottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 8;
            }
        }

        size_u result(mask, 0);
        return result.value;
    }

    size_t is_satisfy_step_123(const Board<BoardX, BoardY> & player,
                               const Board<TargetX, TargetY> target[4],
                               size_t target_len) {
        for (size_t index = 0; index < target_len; index++) {
            size_t mask = is_satisfy_step_123(player, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    size_t is_satisfy_step_456(const Board<BoardX, BoardY> & player,
                               const Board<TargetX, TargetY> & target) {
        size_t mask = 0;

        if (this->data_->s456.phrase1_type == 0) {
            // Top partial
            static const ptrdiff_t TopY = startY;

            if (check_board_is_equal(player, target, 0, TargetX, 0, 2)) {
                count_partial_color_nums(player, 0, BoardX, TopY + 2, BoardY);
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 1;
            }
        }
        else if (this->data_->s456.phrase1_type == 1) {
            // Left partial
            static const ptrdiff_t LeftX = startX;

            if (check_board_is_equal(player, target, 0, 2, 0, TargetY)) {
                count_partial_color_nums(player, LeftX + 2, BoardX, 0, BoardY);
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 2;
            }
        }
        else if (this->data_->s456.phrase1_type == 2) {
            // Right partial
            static const ptrdiff_t RightX = startX + TargetX - 1;

            if (check_board_is_equal(player, target, TargetX - 2, TargetX, 0, TargetY)) {
                count_partial_color_nums(player, 0, startX + 1, 0, BoardY);
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 4;
            }
        }
        else if (this->data_->s456.phrase1_type == 3) {
            // Bottom partial
            static const ptrdiff_t BottomY = startY + TargetY - 1;

            if (check_board_is_equal(player, target, 0, TargetX, TargetY - 2, TargetY)) {
                count_partial_color_nums(player, 0, BoardX, 0, startY + 1);
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 8;
            }
        }
        else {
            assert(false);
        }

        size_u result(mask, 0);
        return result.value;
    }

    size_t is_satisfy_step_456(const Board<BoardX, BoardY> & player,
                               const Board<TargetX, TargetY> target[4],
                               size_t target_len) {
        for (size_t index = 0; index < target_len; index++) {
            if (is_satisfy_step_456(player, target[index]) != 0) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    size_t is_satisfy_step_456_789(const Board<BoardX, BoardY> & player,
                                   const Board<TargetX, TargetY> & target) {
        size_t mask = 0;

        // Check order: down to up
        if (reverse_check_board_is_equal(player, target, 0, TargetX, 0, TargetY)) {
            mask |= 1;
        }

        size_u result(mask, 0);
        return result.value;
    }

    size_t is_satisfy_step_456_789(const Board<BoardX, BoardY> & player,
                                   const Board<TargetX, TargetY> target[4],
                                   size_t target_len) {
        for (size_t index = 0; index < target_len; index++) {
            size_t mask = is_satisfy_step_456_789(player, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    size_t is_satisfy(const Board<BoardX, BoardY> & player,
                      const Board<TargetX, TargetY> & target) {
        if (Step == 1) {
            return is_satisfy_step_1(player, target);
        }
        if (Step == 12) {
            return is_satisfy_step_12(player, target);
        }
        else if (Step == 123) {
            return is_satisfy_step_123(player, target);
        }
        else if (Step == 456) {
            return is_satisfy_step_456_789(player, target);
        }
        else {
            return (size_t)is_satisfy_full(player, target);
        }

        return 0;
    }

    size_t is_satisfy(const Board<BoardX, BoardY> & player,
                      const Board<TargetX, TargetY> target[4],
                      size_t target_len) {
        if (AllowRotate) {
            if (Step == 1) {
                return is_satisfy_step_1(player, target, target_len);
            }
            if (Step == 12) {
                return is_satisfy_step_12(player, target, target_len);
            }
            else if (Step == 123) {
                return is_satisfy_step_123(player, target, target_len);
            }
            else if (Step == 456) {
                return is_satisfy_step_456_789(player, target, target_len);
            }
            else {
                return (size_t)is_satisfy_full(player, target, target_len);
            }
        }
        else {
            return is_satisfy(player, target[0]);
        }

        return 0;
    }

    size_t is_satisfy_all(const Board<BoardX, BoardY> & board,
                          const Board<TargetX, TargetY> target[4],
                          size_t target_len) {
        for (size_t index = 0; index < target_len; index++) {
            size_t mask = is_satisfy(board, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    bool record_phrase1_min_info(size_t depth, size_t rotate_type, size_t satisfy_mask, const stage_type & stage) {
        size_t reached_mask = 0;
        size_t mask = 1;
        size_t phrase1_type = 0;
        while (satisfy_mask != 0) {
            if ((satisfy_mask & mask) == mask) {
                // record min-move phrase1 stage
                this->data_->s123.stage_list[rotate_type][phrase1_type].push_back(stage);

                if (this->data_->s123.min_depth[rotate_type][phrase1_type] != -1) {
                    assert(this->data_->s123.max_depth[rotate_type][phrase1_type] != -1);
                    if ((int)depth >= this->data_->s123.max_depth[rotate_type][phrase1_type]) {
                        reached_mask |= mask;
                    }
                }
                else {
                    if (this->data_->s123.has_solution[rotate_type] == 0) {
                        this->data_->s123.has_solution[rotate_type] = 1;
                        // Update the depth limit
                        this->data_->s123.depth_limit[rotate_type] = std::min(
                            std::max(depth + kMaxSlideDepth, kMinSearchDepth), kMaxSearchDepth);
                    }
                    this->data_->s123.min_depth[rotate_type][phrase1_type] = (int)depth;
                    this->data_->s123.max_depth[rotate_type][phrase1_type] = (int)(depth + kSlideDepth);
                }
            }
            phrase1_type++;
            if (phrase1_type >= MaxPhrase1Type)
                break;
            satisfy_mask &= ~mask;
            mask <<= 1;
        }

        return ((reached_mask & 0x0F) == 0x0F);
    }

    bool solve_full(size_t & rotate_type) {
        size_u result = is_satisfy_full(this->player_board_, this->target_board_, this->target_len_);
        if (result.low != 0) {
            rotate_type = result.high;
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position empty;
        bool found_empty = find_empty(this->player_board_, empty);
        if (found_empty) {
            std::set<Value128> visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = -1;
            start.board = this->data_->player;
            visited.insert(start.board.value128());

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size()) {    
                for (size_t i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    int empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        stage_type next_stage(stage.board);
                        int16_t move_pos = empty_moves[n].pos.value;
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        Value128 board_value = next_stage.board.value128();
                        if (visited.count(board_value) > 0)
                            continue;

                        visited.insert(board_value);

                        next_stage.empty = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.move_path = stage.move_path;
                        Position next_move(stage.empty);
                        next_stage.move_path.push_back(next_move);

                        next_stages.push_back(next_stage);

                        size_u result = is_satisfy_full(next_stage.board, this->target_board_, this->target_len_);
                        if (result.low != 0) {
                            this->move_path_ = next_stage.move_path;
                            assert((depth + 1) == next_stage.move_path.size());
                            rotate_type = result.high;
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
                printf("depth = %u\n", (uint32_t)depth);
                printf("cur.size() = %u, next.size() = %u\n",
                       (uint32_t)(cur_stages.size()),
                       (uint32_t)(next_stages.size()));
                printf("visited.size() = %u\n\n", (uint32_t)(visited.size()));

                std::swap(cur_stages, next_stages);
                next_stages.clear();

                if (exit) {
                    break;
                }
            }

            if (solvable) {
                this->map_used_ = visited.size();
            }
        }

        return solvable;
    }

    bool solve(size_t & out_rotate_type) {
        size_u satisfy_result = is_satisfy(this->player_board_, this->target_board_, this->target_len_);
        if (satisfy_result.low != 0) {
            out_rotate_type = satisfy_result.high;
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position empty;
        bool found_empty = find_empty(this->player_board_, empty);
        if (found_empty) {
            std::set<Value128> visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = -1;
            start.rotate_type = 0;
            start.board = this->player_board_;
            visited.insert(start.board.value128());

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size()) {
                for (size_t i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    int16_t empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        int16_t move_pos = empty_moves[n].pos.value;
                        if (Step == 456) {
                            if (this->data_->s456.locked[move_pos] != 0)
                                continue;
                        }

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        Value128 board_value = next_stage.board.value128();
                        if (visited.count(board_value) > 0)
                            continue;

                        visited.insert(board_value);

                        next_stage.empty.value = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        Position next_move(stage.empty);
                        next_stage.move_path.push_back(next_move);

                        next_stages.push_back(next_stage);

                        size_u satisfy_result = is_satisfy(next_stage.board, this->target_board_, this->target_len_);
                        size_t satisfy_mask = satisfy_result.low;
                        if (satisfy_mask != 0) {
                            solvable = true;
                            if (Step == 1 || Step == 12 || Step == 123) {
                                size_t rotate_type = satisfy_result.high;
                                next_stage.rotate_type = (uint8_t)rotate_type;
                                bool all_reached = record_phrase1_min_info(depth, rotate_type, satisfy_mask, next_stage);
                                if (all_reached) {
                                    exit = true;
                                }
                            }
                            else {
                                this->move_path_ = next_stage.move_path;
                                assert((depth + 1) == next_stage.move_path.size());
                                exit = true;
                                break;
                            }
                        }
                    }
                    if (!(Step == 1 || Step == 12 || Step == 123)) {
                        if (exit) {
                            break;
                        }
                    }
                }

                depth++;
                if (Step == 1 || Step == 12 || Step == 123) {
                    printf("depth = %u\n", (uint32_t)depth);
                    printf("cur.size() = %u, next.size() = %u\n",
                           (uint32_t)(cur_stages.size()), (uint32_t)(next_stages.size()));
                    printf("visited.size() = %u\n\n", (uint32_t)(visited.size()));
                }
                else {
                    //printf(">> %u\n", (uint32_t)depth);
                }

                std::swap(cur_stages, next_stages);
                next_stages.clear();

                if (Step == 1 || Step == 12 || Step == 123) {
                    size_t rotate_done = 0;
                    for (size_t rotate_type = 0; rotate_type < MaxRotateType; rotate_type++) {
                        if (this->data_->s123.depth_limit[rotate_type] != size_t(-1) &&
                            depth > this->data_->s123.depth_limit[rotate_type]) {
                            rotate_done++;
                        }
                    }
                    if (AllowRotate) {
                        if (rotate_done >= MaxRotateType)
                            exit = true;
                    }
                    else {
                        if (rotate_done >= 1)
                            exit = true;
                    }
                }
                else if (Step == 456) {
                    if (depth > this->data_->s456.depth_limit) {
                        exit = true;
                    }
                }

                if (exit) {
                    break;
                }
            }

            this->map_used_ = visited.size();

            if (Step == 1 || Step == 12 || Step == 123) {
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                for (size_t rotate_type = 0; rotate_type < 4; rotate_type++) {
                    for (size_t phrase1_type = 0; phrase1_type < 4; phrase1_type++) {
                        printf("rotate_type = %u, phrase1_type = %u, min_depth = %d, max_depth = %d, stage.size() = %u\n",
                                (uint32_t)rotate_type,
                                (uint32_t)phrase1_type,
                                this->data_->s123.min_depth[rotate_type][phrase1_type],
                                this->data_->s123.max_depth[rotate_type][phrase1_type],
                                (uint32_t)this->data_->s123.stage_list[rotate_type][phrase1_type].size());
                    }
                    printf("\n");
                }
                out_rotate_type = 0;
            }
            else if (Step == 456) {
                //printf("\n");
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                printf("phrase1_type = %u\n", (uint32_t)this->data_->s456.phrase1_type);
                printf("index = %u\n", (uint32_t)(this->data_->s456.index + 1));
                printf("next.size() = %u\n", (uint32_t)cur_stages.size());
                if (solvable) {
                    printf("move_path.size() = %u\n", (uint32_t)this->move_path_.size());
                }
                printf("\n");
            }
        }

        return solvable;
    }

    bool queue_solve(size_t & out_rotate_type) {
        size_u satisfy_result = is_satisfy(this->player_board_, this->target_board_, this->target_len_);
        if (satisfy_result.low != 0) {
            out_rotate_type = satisfy_result.high;
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position empty;
        bool found_empty = find_empty(this->player_board_, empty);
        if (found_empty) {
            std::set<Value128> visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = -1;
            start.rotate_type = 0;
            start.board = this->player_board_;
            visited.insert(start.board.value128());

            std::queue<stage_type> cur_stages;
            std::queue<stage_type> next_stages;

            cur_stages.push(start);

            bool exit = false;
            while (!cur_stages.empty()) {
                do {
                    const stage_type & stage = cur_stages.front();

                    int16_t empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        int16_t move_pos = empty_moves[n].pos.value;
                        if (Step == 456) {
                            if (this->data_->s456.locked[move_pos] != 0)
                                continue;
                        }

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        Value128 board_value = next_stage.board.value128();
                        if (visited.count(board_value) > 0)
                            continue;

                        visited.insert(board_value);

                        next_stage.empty.value = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        Position next_move(stage.empty);
                        next_stage.move_path.push_back(next_move);

                        next_stages.push(next_stage);

                        size_u satisfy_result = is_satisfy(next_stage.board, this->target_board_, this->target_len_);
                        size_t satisfy_mask = satisfy_result.low;
                        if (satisfy_mask != 0) {
                            solvable = true;
                            if (Step == 1 || Step == 12 || Step == 123) {
                                size_t rotate_type = satisfy_result.high;
                                next_stage.rotate_type = (uint8_t)rotate_type;
                                bool all_reached = record_phrase1_min_info(depth, rotate_type, satisfy_mask, next_stage);
                                if (all_reached) {
                                    exit = true;
                                }
                            }
                            else {
                                this->move_path_ = next_stage.move_path;
                                assert((depth + 1) == next_stage.move_path.size());
                                exit = true;
                                break;
                            }
                        }
                    }

                    cur_stages.pop();

                    if (!(Step == 1 || Step == 12 || Step == 123)) {
                        if (exit) {
                            break;
                        }
                    }
                } while (!cur_stages.empty());

                depth++;
                if (Step == 1 || Step == 12 || Step == 123) {
                    printf("depth = %u\n", (uint32_t)depth);
                    printf("cur.size() = %u, next.size() = %u\n",
                           (uint32_t)(cur_stages.size()), (uint32_t)(next_stages.size()));
                    printf("visited.size() = %u\n\n", (uint32_t)(visited.size()));
                }
                else {
                    //printf(">> %u\n", (uint32_t)depth);
                }

                std::swap(cur_stages, next_stages);

                if (Step == 1 || Step == 12 || Step == 123) {
                    size_t rotate_done = 0;
                    for (size_t rotate_type = 0; rotate_type < MaxRotateType; rotate_type++) {
                        if (this->data_->s123.depth_limit[rotate_type] != size_t(-1) &&
                            depth > this->data_->s123.depth_limit[rotate_type]) {
                            rotate_done++;
                        }
                    }
                    if (AllowRotate) {
                        if (rotate_done >= MaxRotateType)
                            exit = true;
                    }
                    else {
                        if (rotate_done >= 1)
                            exit = true;
                    }
                }
                else if (Step == 456) {
                    if (depth > this->data_->s456.depth_limit) {
                        exit = true;
                    }
                }

                if (exit) {
                    break;
                }
            }

            this->map_used_ = visited.size();

            if (Step == 1 || Step == 12 || Step == 123) {
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                for (size_t rotate_type = 0; rotate_type < 4; rotate_type++) {
                    for (size_t phrase1_type = 0; phrase1_type < 4; phrase1_type++) {
                        printf("rotate_type = %u, phrase1_type = %u, min_depth = %d, max_depth = %d, stage.size() = %u\n",
                                (uint32_t)rotate_type,
                                (uint32_t)phrase1_type,
                                this->data_->s123.min_depth[rotate_type][phrase1_type],
                                this->data_->s123.max_depth[rotate_type][phrase1_type],
                                (uint32_t)this->data_->s123.stage_list[rotate_type][phrase1_type].size());
                    }
                    printf("\n");
                }
                out_rotate_type = 0;
            }
            else if (Step == 456) {
                //printf("\n");
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                printf("phrase1_type = %u\n", (uint32_t)this->data_->s456.phrase1_type);
                printf("index = %u\n", (uint32_t)(this->data_->s456.index + 1));
                printf("next.size() = %u\n", (uint32_t)cur_stages.size());
                if (solvable) {
                    printf("move_path.size() = %u\n", (uint32_t)this->move_path_.size());
                }
                printf("\n");
            }
        }

        return solvable;
    }

    bool bitset_solve(size_t & out_rotate_type) {
        size_u satisfy_result = is_satisfy(this->player_board_, this->target_board_, this->target_len_);
        if (satisfy_result.low != 0) {
            out_rotate_type = satisfy_result.high;
            return true;
        }

        bool solvable = false;
        size_t depth = 0;

        Position empty;
        bool found_empty = find_empty(this->player_board_, empty);
        if (found_empty) {
            typedef SparseBitset<Board<BoardX, BoardY>, 3, BoardX * BoardY, 2> bitset_type;
            bitset_type visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = -1;
            start.rotate_type = 0;
            start.board = this->player_board_;
            visited.append(start.board);

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size()) {
                for (size_t i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    int16_t empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_t total_moves = empty_moves.size();
                    for (size_t n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        int16_t move_pos = empty_moves[n].pos.value;
                        if (Step == 456) {
                            if (this->data_->s456.locked[move_pos] != 0)
                                continue;
                        }

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
#if 0
                        if (visited.contains(next_stage.board))
                            continue;

                        visited.append(next_stage.board);
#elif 0
                        typedef typename bitset_type::Container Container;

                        size_t last_layer;
                        Container * last_container;
                        if (visited.contains(next_stage.board, last_layer, last_container))
                            continue;

                        assert(last_layer >= 0 && last_layer <= BoardY);
                        assert(last_container != nullptr);
                        visited.append_new(next_stage.board, last_layer, last_container);
#else
                        bool insert_new = visited.try_append(next_stage.board);
                        if (!insert_new)
                            continue;
#endif
                        next_stage.empty.value = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        Position next_move(stage.empty);
                        next_stage.move_path.push_back(next_move);

                        next_stages.push_back(next_stage);

                        size_u satisfy_result = is_satisfy(next_stage.board, this->target_board_, this->target_len_);
                        size_t satisfy_mask = satisfy_result.low;
                        if (satisfy_mask != 0) {
                            solvable = true;
                            if (Step == 1 || Step == 12 || Step == 123) {
                                size_t rotate_type = satisfy_result.high;
                                next_stage.rotate_type = (uint8_t)rotate_type;
                                bool all_reached = record_phrase1_min_info(depth, rotate_type, satisfy_mask, next_stage);
                                if (all_reached) {
                                    exit = true;
                                }
                            }
                            else {
                                this->move_path_ = next_stage.move_path;
                                assert((depth + 1) == next_stage.move_path.size());
                                exit = true;
                                break;
                            }
                        }
                    }
                    if (!(Step == 1 || Step == 12 || Step == 123)) {
                        if (exit) {
                            break;
                        }
                    }
                }

                depth++;
                if (Step == 1 || Step == 12 || Step == 123) {
                    printf("depth = %u\n", (uint32_t)depth);
                    printf("cur.size() = %u, next.size() = %u\n",
                           (uint32_t)(cur_stages.size()), (uint32_t)(next_stages.size()));
                    printf("visited.size() = %u\n\n", (uint32_t)(visited.size()));
                }
                else {
                    //printf(">> %u\n", (uint32_t)depth);
                }

                std::swap(cur_stages, next_stages);
                next_stages.clear();

                if (Step == 1 || Step == 12 || Step == 123) {
                    size_t rotate_done = 0;
                    for (size_t rotate_type = 0; rotate_type < MaxRotateType; rotate_type++) {
                        if (this->data_->s123.depth_limit[rotate_type] != size_t(-1) &&
                            depth > this->data_->s123.depth_limit[rotate_type]) {
                            rotate_done++;
                        }
                    }
                    if (AllowRotate) {
                        if (rotate_done >= MaxRotateType)
                            exit = true;
                    }
                    else {
                        if (rotate_done >= 1)
                            exit = true;
                    }
                }
                else if (Step == 456) {
                    if (depth > this->data_->s456.depth_limit) {
                        exit = true;
                    }
                }

                if (exit) {
                    break;
                }
            }

            this->map_used_ = visited.size();

            if (Step == 1 || Step == 12 || Step == 123) {
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                for (size_t rotate_type = 0; rotate_type < 4; rotate_type++) {
                    for (size_t phrase1_type = 0; phrase1_type < 4; phrase1_type++) {
                        printf("rotate_type = %u, phrase1_type = %u, min_depth = %d, max_depth = %d, stage.size() = %u\n",
                                (uint32_t)rotate_type,
                                (uint32_t)phrase1_type,
                                this->data_->s123.min_depth[rotate_type][phrase1_type],
                                this->data_->s123.max_depth[rotate_type][phrase1_type],
                                (uint32_t)this->data_->s123.stage_list[rotate_type][phrase1_type].size());
                    }
                    printf("\n");
                }
                out_rotate_type = 0;
            }
            else if (Step == 456) {
                //printf("\n");
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                printf("phrase1_type = %u\n", (uint32_t)this->data_->s456.phrase1_type);
                printf("index = %u\n", (uint32_t)(this->data_->s456.index + 1));
                printf("next.size() = %u\n", (uint32_t)cur_stages.size());
                if (solvable) {
                    printf("move_path.size() = %u\n", (uint32_t)this->move_path_.size());
                }
                printf("\n");
            }

            visited.shutdown();
            visited.display_trie_info();
        }

        return solvable;
    }
};

} // namespace MagicBlock
