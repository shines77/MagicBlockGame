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
#include "Utils.h"

namespace MagicBlock {
namespace v1 {

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t TargetX, std::size_t TargetY,
          bool AllowRotate, std::size_t N_PhaseType, typename Phase2CallBack>
class Solver
{
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

#ifdef NDEBUG
    static const size_type kMinSearchDepth = 15;
    static const size_type kMaxSearchDepth = 27;

    static const size_type kDefaultSearchDepthLimit = 30;

    static const size_type kSlideDepth = 6;
    static const size_type kMaxSlideDepth = 10;
#else
    static const size_type kMinSearchDepth = 15;
    static const size_type kMaxSearchDepth = 21;

    static const size_type kDefaultSearchDepthLimit = 22;

    static const size_type kSlideDepth = 1;
    static const size_type kMaxSlideDepth = 2;
#endif
    static const size_type kSingelColorNums = (BoardX * BoardY - 1) / (Color::Last - 1);

    static const ptrdiff_t startX = (BoardX - TargetX) / 2;
    static const ptrdiff_t startY = (BoardY - TargetY) / 2;

    typedef typename SharedData<BoardX, BoardY, TargetX, TargetY>::stage_type stage_type;
    typedef Phase2CallBack phase2_callback;

private:
    SharedData<BoardX, BoardY, TargetX, TargetY> * data_;

    Board<BoardX, BoardY> player_board_;
    Board<TargetX, TargetY> target_board_[4];

    size_type target_len_;
    size_type rotate_type_;

    int target_colors_[Color::Maximum];  
    int partial_colors_[Color::Maximum];

    std::vector<Position> move_path_;

    size_type map_used_;

    void assert_color(uint8_t color) const {
        assert(color >= Color::Empty && color < Color::Last);
    }

    void count_target_color_nums(const Board<TargetX, TargetY> & target) {
        for (size_type clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->target_colors_[clr] = 0;
        }

        for (size_type y = 0; y < TargetY; y++) {
            for (size_type x = 0; x < TargetX; x++) {
                uint8_t cell = target.cells[y * TargetY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->target_colors_[cell]++;
                }
            }
        }
    }

    void count_partial_target_color_nums(const Board<TargetX, TargetY> & target,
                                         size_type firstTargetX, size_type lastTargetX,
                                         size_type firstTargetY, size_type lastTargetY) {
        for (size_type clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->target_colors_[clr] = 0;
        }

        for (size_type y = firstTargetY; y < lastTargetY; y++) {
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
                uint8_t cell = target.cells[y * TargetY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->target_colors_[cell]++;
                }
            }
        }
    }

    void locked_partial_board(int locked[BoardX * BoardY],
                              size_type firstX, size_type lastX,
                              size_type firstY, size_type lastY) {
        for (size_type y = 0; y < BoardY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_type x = 0; x < BoardX; x++) {
                locked[baseY + x] = 0;
            }
        }

        for (size_type y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_type x = firstX; x < lastX; x++) {
                locked[baseY + x] = 1;
            }
        }
    }

    void init() {
        if (this->is_phase1()) {
            this->player_board_ = this->data_->player_board;
            for (size_type i = 0; i < MAX_ROTATE_TYPE; i++) {
                this->target_board_[i] = this->data_->target_board[i];
            }
            if (AllowRotate)
                this->target_len_ = this->data_->target_len;
            else
                this->target_len_ = 1;

            count_target_color_nums(this->target_board_[0]);

            data_->phase1.init(kDefaultSearchDepthLimit);

            // Reset lock_inited[]
            for (size_type phase1_type = 0; phase1_type < MAX_PHASE1_TYPE; phase1_type++) {
                this->data_->phase2.lock_inited[phase1_type] = 0;
            }
        }
        else if (this->is_phase2()) {
            //
        }
    }

    void init_target_board_locked(size_t rotate_type) {
        if (this->is_phase2()) {
            this->data_->phase2.reset();

            assert(rotate_type >= 0 && rotate_type < MAX_ROTATE_TYPE);
            this->target_board_[0] = this->data_->target_board[rotate_type];
            this->target_len_ = 1;

            size_type phase1_type = this->data_->phase2.phase1_type;
            if (this->data_->phase2.lock_inited[phase1_type] == 0) {
                this->data_->phase2.lock_inited[phase1_type] = 1;
                switch (phase1_type) {
                    case 0:
                        // Top partial
                        count_partial_target_color_nums(this->target_board_[0], 0, TargetX, TargetY - 1, TargetY);
                        locked_partial_board(this->data_->phase2.locked, 0, BoardX, 0, startY + 1);
                        break;
                    case 1:
                        // Left partial
                        count_partial_target_color_nums(this->target_board_[0], TargetX - 1, TargetX, 0, TargetY);
                        locked_partial_board(this->data_->phase2.locked, 0, startX + 1, 0, BoardY);
                        break;
                    case 2:
                        // Right partial
                        count_partial_target_color_nums(this->target_board_[0], 0, 1, 0, TargetY);
                        locked_partial_board(this->data_->phase2.locked, startX + TargetX - 1, BoardX, 0, BoardY);
                        break;
                    case 3:
                        // Bottom partial
                        count_partial_target_color_nums(this->target_board_[0], 0, TargetX, 0, 1);
                        locked_partial_board(this->data_->phase2.locked, 0, BoardX, startY + TargetY - 1, BoardY);
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
        }
    }

public:
    Solver(SharedData<BoardX, BoardY, TargetX, TargetY> * data)
        : data_(data), target_len_(0), rotate_type_(0), map_used_(0) {
        this->init();
    }

    ~Solver() {}

    bool is_phase1() const {
        return (N_PhaseType == PhaseType::Phase1_1 ||
                N_PhaseType == PhaseType::Phase1_12 ||
                N_PhaseType == PhaseType::Phase1_123);
    }

    bool is_phase2() const {
        return (N_PhaseType == PhaseType::Phase2);
    }

    size_type getMinSteps() const {
        return this->move_path_.size();
    }

    const std::vector<Position> & getMovePath() const {
        return this->move_path_;
    }

    size_type getMapUsed() const {
        return this->map_used_;
    }

    void setPlayerBoard(const Board<BoardX, BoardY> & board) {
        this->player_board_ = board;
    }

    void setRotateType(size_type rotate_type) {
        assert(rotate_type >= 0 && rotate_type < MAX_ROTATE_TYPE);
        this->rotate_type_ = rotate_type;
        init_target_board_locked(rotate_type);
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

    void count_reverse_partial_color_nums(const Board<BoardX, BoardY> & board,
                                          size_type firstX, size_type lastX,
                                          size_type firstY, size_type lastY) {
        this->partial_colors_[Color::Empty] = 0;
        for (size_type clr = Color::First; clr < Color::Last; clr++) {
            this->partial_colors_[clr] = kSingelColorNums;
        }

        for (size_type y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_type x = firstX; x < lastX; x++) {
                uint8_t cell = board.cells[baseY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Last) {
                    this->partial_colors_[cell]--;
                }
            }
        }
    }

    void count_partial_color_nums(const Board<BoardX, BoardY> & board,
                                  size_type firstX, size_type lastX,
                                  size_type firstY, size_type lastY) {
        for (size_type clr = Color::Empty; clr < Color::Last; clr++) {
            this->partial_colors_[clr] = 0;
        }

        for (size_type y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardY;
            for (size_type x = firstX; x < lastX; x++) {
                uint8_t cell = board.cells[baseY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Last) {
                    this->partial_colors_[cell]++;
                }
            }
        }
    }

    bool check_partial_color_nums() const {
        if (this->is_phase2()) {
            for (size_type clr = Color::Empty; clr < Color::Last; clr++) {
                if (this->partial_colors_[clr] < this->target_colors_[clr]) {
                    return false;
                }
            }
        }
        else {
            for (size_type clr = Color::First; clr < Color::Last; clr++) {
                if (this->partial_colors_[clr] < this->target_colors_[clr]) {
                    return false;
                }
            }
        }
        return true;
    }

    bool is_satisfy_full(const Board<BoardX, BoardY> & player,
                         const Board<TargetX, TargetY> & target) const {
        for (size_type y = 0; y < TargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_type x = 0; x < TargetX; x++) {
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

    size_type is_satisfy_full(const Board<BoardX, BoardY> & player,
                              const Board<TargetX, TargetY> target[4],
                              size_type target_len) const {
        for (size_type index = 0; index < target_len; index++) {
            if (is_satisfy_full(player, target[index])) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    size_type is_satisfy_phase1_1(const Board<BoardX, BoardY> & player,
                             const Board<TargetX, TargetY> & target) {
        static const ptrdiff_t startX = (BoardX - TargetX) / 2;
        static const ptrdiff_t startY = (BoardY - TargetY) / 2;

        size_type mask = 0;

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

    size_type is_satisfy_phase1_1(const Board<BoardX, BoardY> & player,
                                const Board<TargetX, TargetY> target[4],
                                size_type target_len) {
        for (size_type index = 0; index < target_len; index++) {
            size_type mask = is_satisfy_phase1_1(player, target[index]);
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
                              size_type firstTargetX, size_type lastTargetX,
                              size_type firstTargetY, size_type lastTargetY) {
        for (size_type y = firstTargetY; y < lastTargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
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
    size_type check_board_is_equal(const Board<BoardX, BoardY> & player,
                                   const Board<TargetX, TargetY> target[4],
                                   size_type target_len,
                                   size_type firstTargetX, size_type lastTargetX,
                                   size_type firstTargetY, size_type lastTargetY) {
        for (size_type index = 0; index < target_len; index++) {
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
                                      size_type firstTargetX, size_type lastTargetX,
                                      size_type firstTargetY, size_type lastTargetY) {
        for (ptrdiff_t y = ptrdiff_t(lastTargetY - 1); y >= ptrdiff_t(firstTargetX); y--) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (startY + y) * BoardY;
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
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
    size_type reverse_check_board_is_equal(const Board<BoardX, BoardY> & player,
                                           const Board<TargetX, TargetY> & target,
                                           size_type target_len,
                                           size_type firstTargetX, size_type lastTargetX,
                                           size_type firstTargetY, size_type lastTargetY) {
        for (size_type index = 0; index < target_len; index++) {
            if (reverse_check_board_is_equal(player, target[index],
                firstTargetX, lastTargetX, firstTargetY, lastTargetY)) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    size_type is_satisfy_phase1_12(const Board<BoardX, BoardY> & player,
                                 const Board<TargetX, TargetY> & target) {
        size_type mask = 0;

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

    size_type is_satisfy_phase1_12(const Board<BoardX, BoardY> & player,
                                 const Board<TargetX, TargetY> target[4],
                                 size_type target_len) {
        for (size_type index = 0; index < target_len; index++) {
            size_type mask = is_satisfy_phase1_12(player, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    size_type is_satisfy_phase1_123(const Board<BoardX, BoardY> & player,
                                  const Board<TargetX, TargetY> & target) {
        size_type mask = 0;

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

    size_type is_satisfy_phase1_123(const Board<BoardX, BoardY> & player,
                                  const Board<TargetX, TargetY> target[4],
                                  size_type target_len) {
        for (size_type index = 0; index < target_len; index++) {
            size_type mask = is_satisfy_phase1_123(player, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    size_type is_satisfy_step_456(const Board<BoardX, BoardY> & player,
                                  const Board<TargetX, TargetY> & target) {
        size_type mask = 0;

        if (this->data_->phase2.phase1_type == 0) {
            // Top partial
            static const ptrdiff_t TopY = startY;

            if (check_board_is_equal(player, target, 0, TargetX, 0, 2)) {
                count_partial_color_nums(player, 0, BoardX, TopY + 2, BoardY);
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 1;
            }
        }
        else if (this->data_->phase2.phase1_type == 1) {
            // Left partial
            static const ptrdiff_t LeftX = startX;

            if (check_board_is_equal(player, target, 0, 2, 0, TargetY)) {
                count_partial_color_nums(player, LeftX + 2, BoardX, 0, BoardY);
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 2;
            }
        }
        else if (this->data_->phase2.phase1_type == 2) {
            // Right partial
            static const ptrdiff_t RightX = startX + TargetX - 1;

            if (check_board_is_equal(player, target, TargetX - 2, TargetX, 0, TargetY)) {
                count_partial_color_nums(player, 0, startX + 1, 0, BoardY);
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 4;
            }
        }
        else if (this->data_->phase2.phase1_type == 3) {
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

    size_type is_satisfy_step_456(const Board<BoardX, BoardY> & player,
                                  const Board<TargetX, TargetY> target[4],
                                  size_type target_len) {
        for (size_type index = 0; index < target_len; index++) {
            if (is_satisfy_step_456(player, target[index]) != 0) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    size_type is_satisfy_phase2(const Board<BoardX, BoardY> & player,
                                      const Board<TargetX, TargetY> & target) {
        size_type mask = 0;

        // Check order: down to up
        if (reverse_check_board_is_equal(player, target, 0, TargetX, 0, TargetY)) {
            mask |= 1;
        }

        size_u result(mask, 0);
        return result.value;
    }

    size_type is_satisfy_phase2(const Board<BoardX, BoardY> & player,
                                      const Board<TargetX, TargetY> target[4],
                                      size_type target_len) {
        for (size_type index = 0; index < target_len; index++) {
            size_type mask = is_satisfy_phase2(player, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    size_type is_satisfy(const Board<BoardX, BoardY> & player,
                         const Board<TargetX, TargetY> & target) {
        if (N_PhaseType == PhaseType::Phase1_1) {
            return is_satisfy_phase1_1(player, target);
        }
        if (N_PhaseType == PhaseType::Phase1_12) {
            return is_satisfy_phase1_12(player, target);
        }
        else if (N_PhaseType == PhaseType::Phase1_123) {
            return is_satisfy_phase1_123(player, target);
        }
        else if (N_PhaseType == PhaseType::Phase2) {
            return is_satisfy_phase2(player, target);
        }
        else {
            return (size_type)is_satisfy_full(player, target);
        }

        return 0;
    }

    size_type is_satisfy(const Board<BoardX, BoardY> & player,
                         const Board<TargetX, TargetY> target[4],
                         size_type target_len) {
        if (AllowRotate) {
            if (N_PhaseType == PhaseType::Phase1_1) {
                return is_satisfy_phase1_1(player, target, target_len);
            }
            if (N_PhaseType == PhaseType::Phase1_12) {
                return is_satisfy_phase1_12(player, target, target_len);
            }
            else if (N_PhaseType == PhaseType::Phase1_123) {
                return is_satisfy_phase1_123(player, target, target_len);
            }
            else if (N_PhaseType == PhaseType::Phase2) {
                return is_satisfy_phase2(player, target, target_len);
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

    size_type is_satisfy_all(const Board<BoardX, BoardY> & board,
                             const Board<TargetX, TargetY> target[4],
                             size_type target_len) {
        for (size_type index = 0; index < target_len; index++) {
            size_type mask = is_satisfy(board, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    bool record_phase1_min_info(size_type depth, size_type rotate_type, size_type satisfy_mask, const stage_type & stage) {
        size_type reached_mask = 0;
        size_type mask = 1;
        size_type phase1_type = 0;
        while (satisfy_mask != 0) {
            if ((satisfy_mask & mask) == mask) {
                // record min-move phrase1 stage
                this->data_->phase1.stage_list[rotate_type][phase1_type].push_back(stage);

                if (this->data_->phase1.min_depth[rotate_type][phase1_type] != -1) {
                    assert(this->data_->phase1.max_depth[rotate_type][phase1_type] != -1);
                    if ((int)depth >= this->data_->phase1.max_depth[rotate_type][phase1_type]) {
                        reached_mask |= mask;
                    }
                }
                else {
                    if (this->data_->phase1.has_solution[rotate_type] == 0) {
                        this->data_->phase1.has_solution[rotate_type] = 1;
                        // Update the depth limit
                        this->data_->phase1.depth_limit[rotate_type] = std::min(
                            std::max(depth + kMaxSlideDepth, kMinSearchDepth), kMaxSearchDepth);
                    }
                    this->data_->phase1.min_depth[rotate_type][phase1_type] = (int)depth;
                    this->data_->phase1.max_depth[rotate_type][phase1_type] = (int)(depth + kSlideDepth);
                }
            }
            phase1_type++;
            if (phase1_type >= MAX_PHASE1_TYPE)
                break;
            satisfy_mask &= ~mask;
            mask <<= 1;
        }

        return ((reached_mask & 0x0F) == 0x0F);
    }

    bool solve_full(size_type & rotate_type) {
        size_u result = is_satisfy_full(this->player_board_, this->target_board_, this->target_len_);
        if (result.low != 0) {
            rotate_type = result.high;
            return true;
        }

        bool solvable = false;
        size_type depth = 0;

        Position empty;
        bool found_empty = find_empty(this->player_board_, empty);
        if (found_empty) {
            std::set<Value128> visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = uint8_t(-1);
            start.board = this->data_->player;
            visited.insert(start.board.value128());

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size()) {    
                for (size_type i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    uint8_t empty_pos = stage.empty;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_type total_moves = empty_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        stage_type next_stage(stage.board);
                        uint8_t move_pos = empty_moves[n].pos;
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

    bool solve(size_type & out_rotate_type) {
        size_u satisfy_result = is_satisfy(this->player_board_, this->target_board_, this->target_len_);
        if (satisfy_result.low != 0) {
            out_rotate_type = satisfy_result.high;
            return true;
        }

        bool solvable = false;
        size_type depth = 0;

        Position empty;
        bool found_empty = find_empty(this->player_board_, empty);
        if (found_empty) {
            std::set<Value128> visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = uint8_t(-1);
            start.rotate_type = 0;
            start.board = this->player_board_;
            visited.insert(start.board.value128());

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size()) {
                for (size_type i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    uint8_t empty_pos = stage.empty;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_type total_moves = empty_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = empty_moves[n].pos;
                        if (N_PhaseType == PhaseType::Phase2) {
                            if (this->data_->phase2.locked[move_pos] != 0)
                                continue;
                        }

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        Value128 board_value = next_stage.board.value128();
                        if (visited.count(board_value) > 0)
                            continue;

                        visited.insert(board_value);

                        next_stage.empty = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        Position next_move(stage.empty);
                        next_stage.move_path.push_back(next_move);

                        next_stages.push_back(next_stage);

                        size_u satisfy_result = is_satisfy(next_stage.board, this->target_board_, this->target_len_);
                        size_type satisfy_mask = satisfy_result.low;
                        if (satisfy_mask != 0) {
                            solvable = true;
                            if (this->is_phase1()) {
                                size_type rotate_type = satisfy_result.high;
                                next_stage.rotate_type = (uint8_t)rotate_type;
                                bool all_reached = record_phase1_min_info(depth, rotate_type, satisfy_mask, next_stage);
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
                    if (!(this->is_phase1())) {
                        if (exit) {
                            break;
                        }
                    }
                }

                depth++;
                if (this->is_phase1()) {
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

                if (this->is_phase1()) {
                    size_type rotate_done = 0;
                    for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                        if (this->data_->phase1.depth_limit[rotate_type] != size_t(-1) &&
                            depth > this->data_->phase1.depth_limit[rotate_type]) {
                            rotate_done++;
                        }
                    }
                    if (AllowRotate) {
                        if (rotate_done >= MAX_ROTATE_TYPE)
                            exit = true;
                    }
                    else {
                        if (rotate_done >= 1)
                            exit = true;
                    }
                }
                else if (this->is_phase2()) {
                    if (depth > this->data_->phase2.depth_limit) {
                        exit = true;
                    }
                }

                if (exit) {
                    break;
                }
            }

            this->map_used_ = visited.size();

            if (this->is_phase1()) {
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                for (size_type rotate_type = 0; rotate_type < 4; rotate_type++) {
                    for (size_type phase1_type = 0; phase1_type < 4; phase1_type++) {
                        printf("rotate_type = %u, phase1_type = %u, min_depth = %d, max_depth = %d, stage.size() = %u\n",
                                (uint32_t)rotate_type,
                                (uint32_t)phase1_type,
                                this->data_->phase1.min_depth[rotate_type][phase1_type],
                                this->data_->phase1.max_depth[rotate_type][phase1_type],
                                (uint32_t)this->data_->phase1.stage_list[rotate_type][phase1_type].size());
                    }
                    printf("\n");
                }
                out_rotate_type = 0;
            }
            else if (this->is_phase2()) {
                //printf("\n");
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                printf("phase1_type = %u\n", (uint32_t)this->data_->phase2.phase1_type);
                printf("index = %u\n", (uint32_t)(this->data_->phase2.index + 1));
                printf("next.size() = %u\n", (uint32_t)cur_stages.size());
                if (solvable) {
                    printf("move_path.size() = %u\n", (uint32_t)this->move_path_.size());
                }
                printf("\n");
            }
        }

        return solvable;
    }

    bool queue_solve(size_type & out_rotate_type) {
        size_u satisfy_result = is_satisfy(this->player_board_, this->target_board_, this->target_len_);
        if (satisfy_result.low != 0) {
            out_rotate_type = satisfy_result.high;
            return true;
        }

        bool solvable = false;
        size_type depth = 0;

        Position empty;
        bool found_empty = find_empty(this->player_board_, empty);
        if (found_empty) {
            std::set<Value128> visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = uint8_t(-1);
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

                    uint8_t empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_type total_moves = empty_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = empty_moves[n].pos;
                        if (this->is_phase2()) {
                            if (this->data_->phase2.locked[move_pos] != 0)
                                continue;
                        }

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
                        Value128 board_value = next_stage.board.value128();
                        if (visited.count(board_value) > 0)
                            continue;

                        visited.insert(board_value);

                        next_stage.empty = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        Position next_move(stage.empty);
                        next_stage.move_path.push_back(next_move);

                        next_stages.push(next_stage);

                        size_u satisfy_result = is_satisfy(next_stage.board, this->target_board_, this->target_len_);
                        size_type satisfy_mask = satisfy_result.low;
                        if (satisfy_mask != 0) {
                            solvable = true;
                            if (this->is_phase1()) {
                                size_type rotate_type = satisfy_result.high;
                                next_stage.rotate_type = (uint8_t)rotate_type;
                                bool all_reached = record_phase1_min_info(depth, rotate_type, satisfy_mask, next_stage);
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

                    if (!(this->is_phase1())) {
                        if (exit) {
                            break;
                        }
                    }
                } while (!cur_stages.empty());

                depth++;
                if (this->is_phase1()) {
                    printf("depth = %u\n", (uint32_t)depth);
                    printf("cur.size() = %u, next.size() = %u\n",
                           (uint32_t)(cur_stages.size()), (uint32_t)(next_stages.size()));
                    printf("visited.size() = %u\n\n", (uint32_t)(visited.size()));
                }
                else {
                    //printf(">> %u\n", (uint32_t)depth);
                }

                std::swap(cur_stages, next_stages);

                if (this->is_phase1()) {
                    size_type rotate_done = 0;
                    for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                        if (this->data_->phase1.depth_limit[rotate_type] != size_t(-1) &&
                            depth > this->data_->phase1.depth_limit[rotate_type]) {
                            rotate_done++;
                        }
                    }
                    if (AllowRotate) {
                        if (rotate_done >= MAX_ROTATE_TYPE)
                            exit = true;
                    }
                    else {
                        if (rotate_done >= 1)
                            exit = true;
                    }
                }
                else if (this->is_phase2()) {
                    if (depth > this->data_->phase2.depth_limit) {
                        exit = true;
                    }
                }

                if (exit) {
                    break;
                }
            }

            this->map_used_ = visited.size();

            if (this->is_phase1()) {
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                for (size_type rotate_type = 0; rotate_type < 4; rotate_type++) {
                    for (size_type phase1_type = 0; phase1_type < 4; phase1_type++) {
                        printf("rotate_type = %u, phase1_type = %u, min_depth = %d, max_depth = %d, stage.size() = %u\n",
                                (uint32_t)rotate_type,
                                (uint32_t)phase1_type,
                                this->data_->phase1.min_depth[rotate_type][phase1_type],
                                this->data_->phase1.max_depth[rotate_type][phase1_type],
                                (uint32_t)this->data_->phase1.stage_list[rotate_type][phase1_type].size());
                    }
                    printf("\n");
                }
                out_rotate_type = 0;
            }
            else if (this->is_phase2()) {
                //printf("\n");
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                printf("phase1_type = %u\n", (uint32_t)this->data_->phase2.phase1_type);
                printf("index = %u\n", (uint32_t)(this->data_->phase2.index + 1));
                printf("next.size() = %u\n", (uint32_t)cur_stages.size());
                if (solvable) {
                    printf("move_path.size() = %u\n", (uint32_t)this->move_path_.size());
                }
                printf("\n");
            }
        }

        return solvable;
    }

    bool call_phase2_search(size_type depth, size_type rotate_type, size_type satisfy_mask,
                            const stage_type & stage, phase2_callback & phase2_search) {
        size_type reached_mask = 0;
        size_type mask = 1;
        size_type phase1_type = 0;
        while (satisfy_mask != 0) {
            if ((satisfy_mask & mask) == mask) {
                // record min-move phrase1 stage
                this->data_->phase1.stage_list[rotate_type][phase1_type].push_back(stage);

                if (this->data_->phase1.min_depth[rotate_type][phase1_type] != -1) {
                    assert(this->data_->phase1.max_depth[rotate_type][phase1_type] != -1);
                    if ((int)depth >= this->data_->phase1.max_depth[rotate_type][phase1_type]) {
                        reached_mask |= mask;
                    }
                }
                else {
                    if (this->data_->phase1.has_solution[rotate_type] == 0) {
                        this->data_->phase1.has_solution[rotate_type] = 1;
                        // Update the depth limit
                        this->data_->phase1.depth_limit[rotate_type] = std::min(
                            std::max(depth + kMaxSlideDepth, kMinSearchDepth), kMaxSearchDepth);
                    }
                    this->data_->phase1.min_depth[rotate_type][phase1_type] = (int)depth;
                    this->data_->phase1.max_depth[rotate_type][phase1_type] = (int)(depth + kSlideDepth);
                }

                // call phase2_search()
                if (phase2_search) {
                    bool phase2_solvable = phase2_search(rotate_type, phase1_type, stage);
                }
            }
            phase1_type++;
            if (phase1_type >= MAX_PHASE1_TYPE)
                break;
            satisfy_mask &= ~mask;
            mask <<= 1;
        }

        return ((reached_mask & 0x0F) == 0x0F);
    }

    bool bitset_solve(size_type & out_rotate_type, phase2_callback & phase2_search) {
        size_u satisfy_result = is_satisfy(this->player_board_, this->target_board_, this->target_len_);
        if (satisfy_result.low != 0) {
            out_rotate_type = satisfy_result.high;
            return true;
        }

        bool solvable = false;
        size_type depth = 0;

        Position empty;
        bool found_empty = find_empty(this->player_board_, empty);
        if (found_empty) {
            typedef SparseBitset<Board<BoardX, BoardY>, 3, BoardX * BoardY, 2> bitset_type;
            bitset_type visited;

            stage_type start;
            start.empty = empty;
            start.last_dir = uint8_t(-1);
            start.rotate_type = 0;
            start.board = this->player_board_;
            visited.append(start.board);

            std::vector<stage_type> cur_stages;
            std::vector<stage_type> next_stages;

            cur_stages.push_back(start);

            bool exit = false;
            while (cur_stages.size()) {
                for (size_type i = 0; i < cur_stages.size(); i++) {
                    const stage_type & stage = cur_stages[i];

                    uint8_t empty_pos = stage.empty.value;
                    const std::vector<Move> & empty_moves = this->data_->empty_moves[empty_pos];
                    size_type total_moves = empty_moves.size();
                    for (size_type n = 0; n < total_moves; n++) {
                        uint8_t cur_dir = empty_moves[n].dir;
                        if (cur_dir == stage.last_dir)
                            continue;

                        uint8_t move_pos = empty_moves[n].pos;
                        if (this->is_phase2()) {
                            if (this->data_->phase2.locked[move_pos] != 0)
                                continue;
                        }

                        stage_type next_stage(stage.board);
                        std::swap(next_stage.board.cells[empty_pos], next_stage.board.cells[move_pos]);
#if 1
                        bool insert_new = visited.try_append(next_stage.board);
                        if (!insert_new)
                            continue;
#elif 0
                        if (visited.contains(next_stage.board))
                            continue;

                        visited.append(next_stage.board);
#else
                        typedef typename bitset_type::Container Container;

                        size_type last_layer;
                        Container * last_container;
                        if (visited.contains(next_stage.board, last_layer, last_container))
                            continue;

                        assert(last_layer >= 0 && last_layer <= BoardY);
                        assert(last_container != nullptr);
                        visited.append_new(next_stage.board, last_layer, last_container);
#endif
                        next_stage.empty = move_pos;
                        next_stage.last_dir = cur_dir;
                        next_stage.rotate_type = 0;
                        next_stage.move_path = stage.move_path;
                        Position next_move(stage.empty);
                        next_stage.move_path.push_back(next_move);

                        next_stages.push_back(next_stage);

                        size_u satisfy_result = is_satisfy(next_stage.board, this->target_board_, this->target_len_);
                        size_type satisfy_mask = satisfy_result.low;
                        if (satisfy_mask != 0) {
                            solvable = true;
                            if (this->is_phase1()) {
                                size_type rotate_type = satisfy_result.high;
                                next_stage.rotate_type = (uint8_t)rotate_type;
                                bool all_reached = call_phase2_search(depth, rotate_type, satisfy_mask,
                                                                      next_stage, phase2_search);
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
                    if (!(this->is_phase1())) {
                        if (exit) {
                            break;
                        }
                    }
                }

                depth++;
                if (this->is_phase1()) {
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

                if (this->is_phase1()) {
                    size_type rotate_done = 0;
                    for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                        if (this->data_->phase1.depth_limit[rotate_type] != size_t(-1) &&
                            depth > this->data_->phase1.depth_limit[rotate_type]) {
                            rotate_done++;
                        }
                    }
                    if (AllowRotate) {
                        if (rotate_done >= MAX_ROTATE_TYPE)
                            exit = true;
                    }
                    else {
                        if (rotate_done >= 1)
                            exit = true;
                    }
                }
                else if (this->is_phase2()) {
                    if (depth > this->data_->phase2.depth_limit) {
                        exit = true;
                    }
                }

                if (exit) {
                    break;
                }
            }

            this->map_used_ = visited.size();

            if (this->is_phase1()) {
                printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                for (size_type rotate_type = 0; rotate_type < MAX_ROTATE_TYPE; rotate_type++) {
                    for (size_type phase1_type = 0; phase1_type < MAX_PHASE1_TYPE; phase1_type++) {
                        printf("rotate_type = %u, phase1_type = %u, min_depth = %d, max_depth = %d, stage.size() = %u\n",
                                (uint32_t)rotate_type,
                                (uint32_t)phase1_type,
                                this->data_->phase1.min_depth[rotate_type][phase1_type],
                                this->data_->phase1.max_depth[rotate_type][phase1_type],
                                (uint32_t)this->data_->phase1.stage_list[rotate_type][phase1_type].size());
                    }
                    printf("\n");
                }
                out_rotate_type = 0;
            }
            else if (this->is_phase2()) {
                //printf("\n");
                if (solvable) {
                    printf("Solvable: %s\n\n", (solvable ? "true" : "false"));
                    printf("rotate_type = %u\n", (uint32_t)this->data_->phase2.rotate_type);
                    printf("phase1_type = %u\n", (uint32_t)this->data_->phase2.phase1_type);
                    printf("index = %u\n", (uint32_t)(this->data_->phase2.index + 1));
                    printf("next.size() = %u\n", (uint32_t)cur_stages.size());
                    if (solvable) {
                        printf("move_path.size() = %u\n", (uint32_t)this->move_path_.size());
                    }
                    printf("\n");
                }
            }

            visited.display_trie_info();
        }

        return solvable;
    }
};

} // namespace v1
} // namespace MagicBlock
