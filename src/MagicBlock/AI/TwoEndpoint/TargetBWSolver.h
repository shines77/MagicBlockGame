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

#include "MagicBlock/AI/Constant.h"
#include "MagicBlock/AI/Color.h"
#include "MagicBlock/AI/Move.h"
#include "MagicBlock/AI/Value128.h"
#include "MagicBlock/AI/Board.h"
#include "MagicBlock/AI/Stage.h"
#include "MagicBlock/AI/SharedData.h"
#include "MagicBlock/AI/Utils.h"

namespace MagicBlock {
namespace AI {
namespace TwoEndpoint {

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t TargetX, std::size_t TargetY,
          bool AllowRotate, std::size_t N_SolverType,
          typename Phase2CallBack>
class TargetBWSolver
{
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

    typedef TargetBWSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack>  this_type;

    typedef SharedData<BoardX, BoardY, TargetX, TargetY>    shared_data_type;
    typedef typename shared_data_type::stage_type           stage_type;
    typedef Phase2CallBack                                  phase2_callback;

    static const size_type kSingelColorNums = (BoardX * BoardY - 1) / (Color::Last - 1);

    static const ptrdiff_t kStartX = (BoardX - TargetX) / 2;
    static const ptrdiff_t kStartY = (BoardY - TargetY) / 2;

protected:
    shared_data_type * data_;

    Board<BoardX, BoardY>   player_board_[4];
    Board<BoardX, BoardX>   target_board_;
    Board<TargetX, TargetX> fw_target_board_[4];

    size_type target_len_;
    size_type rotate_type_;

    int player_colors_[Color::Maximum];
    int target_colors_[Color::Maximum];
    int partial_colors_[Color::Maximum];

    std::vector<Position> move_path_;
    std::vector<MoveInfo> answer_;

    size_type map_used_;

    void init() {
        assert(this->data_ != nullptr);

        for (size_type i = 0; i < MAX_ROTATE_TYPE; i++) {
            this->fw_target_board_[i] = this->data_->target_board[i];
        }
        if (AllowRotate)
            this->target_len_ = this->data_->target_len;
        else
            this->target_len_ = 1;

        this->count_target_color_nums(this->fw_target_board_[0]);

        this->setPlayerBoard(this->data_->target_board);
        this->setTargetBoard(this->data_->player_board);
    }

public:
    TargetBWSolver(shared_data_type * data)
        : data_(data), target_len_(0), rotate_type_(0), map_used_(0) {
        this->init();
    }

    ~TargetBWSolver() {
        this->destory();
    }

    void destory() {
        // TODO:
    }

    size_type getMinSteps() const {
        return this->move_path_.size();
    }

    const std::vector<Position> & getMovePath() const {
        return this->move_path_;
    }

    const std::vector<MoveInfo> & getAnswer() const {
        return this->answer_;
    }

    size_type getMapUsed() const {
        return this->map_used_;
    }

    Board<BoardX, BoardY> & getPlayerBoard() {
        return this->player_board_[0];
    }

    const Board<BoardX, BoardY> & getPlayerBoard() const {
        return this->player_board_[0];
    }

    Board<BoardX, BoardY> & getTargetBoard() {
        return this->target_board_;
    }

    const Board<BoardX, BoardY> & getTargetBoard() const {
        return this->target_board_;
    }

    Board<TargetX, TargetY> & getFwTargetBoard() {
        return this->fw_target_board_[0];
    }

    const Board<TargetX, TargetY> & getFwTargetBoard() const {
        return this->fw_target_board_[0];
    }

    size_type getRotateType() const {
        return this->rotate_type_;
    }

    void setPlayerBoard(const Board<TargetX, TargetY> target_board[4],
                        size_type rotate_type = size_type(-1)) {
        if (AllowRotate) {
            rotate_type = size_type(-1);
            assert(this->target_len_ > 1);
            for (size_type i = 0; i < this->target_len_; i++) {
                this->setPlayerBoardFromTargetBoard(this->player_board_[i],
                                                    this->data_->target_board[i]);
            }
        }
        else {
            if (rotate_type >= MAX_ROTATE_TYPE)
                rotate_type = 0;
            this->setPlayerBoardFromTargetBoard(this->player_board_[0],
                                                this->data_->target_board[rotate_type]);
            assert(this->target_len_ == 1);
            this->target_len_ = 1;
        }
        this->setRotateType(rotate_type);
    }

    void setTargetBoard(const Board<BoardX, BoardY> & player_board) {
        this->target_board_ = player_board;
    }

protected:
    void assert_color(uint8_t color) const {
        assert(color >= Color::Empty && color < Color::Maximum);
    }

    void setRotateType(size_type rotate_type) {
        assert((rotate_type >= 0 && rotate_type < MAX_ROTATE_TYPE) || (rotate_type == size_type(-1)));
        this->rotate_type_ = rotate_type;
    }

    void setPlayerBoardFromTargetBoard(Board<BoardX, BoardY> & player_board,
                                       const Board<TargetX, TargetY> & target_board) {
        // Fill the Color::Unknown to player board
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                player_board.cells[y * BoardY + x] = Color::Unknown;
            }
        }

        // Copy input target board to player board
        for (size_type y = 0; y < TargetY; y++) {
            for (size_type x = 0; x < TargetX; x++) {
                uint8_t cell = target_board.cells[y * TargetY + x];
                assert_color(cell);
                size_type new_pos = (y + kStartY) * BoardY + (x + kStartX);
                player_board.cells[new_pos] = cell;
            }
        }
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

    bool find_unknown(const Board<BoardX, BoardY> & board,
                      size_type start_pos, Position & unknown_pos) const {
        for (size_type pos = start_pos; pos < (BoardX * BoardY); pos++) {
            uint8_t cell = board.cells[pos];
            if (cell == Color::Unknown) {
                unknown_pos = pos;
                return true;
            }
        }
        return false;
    }

    void find_all_colors(const Board<BoardX, BoardY> & board,
                         size_type color, std::vector<Position> & pos_list) {
        for (size_type pos = 0; pos < (BoardX * BoardY); pos++) {
            uint8_t cell = board.cells[pos];
            if (cell == Color::Unknown) {
                pos_list.push_back(pos);
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

    void count_partial_color_nums_reverse(const Board<BoardX, BoardY> & board,
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

    void count_target_color_nums(const Board<BoardX, BoardY> & target) {
        for (size_type clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->target_colors_[clr] = 0;
        }

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t cell = target.cells[y * BoardY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->target_colors_[cell]++;
                }
            }
        }
    }

    void count_partial_target_color_nums(const Board<BoardX, BoardY> & target,
                                         size_type firstTargetX, size_type lastTargetX,
                                         size_type firstTargetY, size_type lastTargetY) {
        for (size_type clr = Color::Empty; clr < Color::Maximum; clr++) {
            this->target_colors_[clr] = 0;
        }

        for (size_type y = firstTargetY; y < lastTargetY; y++) {
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
                uint8_t cell = target.cells[y * BoardY + x];
                assert_color(cell);
                if (cell >= Color::Empty && cell < Color::Maximum) {
                    this->target_colors_[cell]++;
                }
            }
        }
    }

    // Check order: up to down
    bool partial_target_is_satisfy(const Board<BoardX, BoardY> & player,
                                   const Board<TargetX, TargetY> & target,
                                   size_type firstTargetX, size_type lastTargetX,
                                   size_type firstTargetY, size_type lastTargetY) {
        for (size_type y = firstTargetY; y < lastTargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (kStartY + y) * BoardY;
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
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

    // Check order: up to down
    size_type partial_target_is_satisfy(const Board<BoardX, BoardY> & player,
                                        const Board<TargetX, TargetY> target[4],
                                        size_type target_len,
                                        size_type firstTargetX, size_type lastTargetX,
                                        size_type firstTargetY, size_type lastTargetY) {
        for (size_type index = 0; index < target_len; index++) {
            if (this->partial_target_is_satisfy(player, target[index],
                                                firstTargetX, lastTargetX,
                                                firstTargetY, lastTargetY)) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    // Check order: down to up
    bool partial_target_is_satisfy_reverse(const Board<BoardX, BoardY> & player,
                                           const Board<TargetX, TargetY> & target,
                                           size_type firstTargetX, size_type lastTargetX,
                                           size_type firstTargetY, size_type lastTargetY) {
        for (ptrdiff_t y = ptrdiff_t(lastTargetY - 1); y >= ptrdiff_t(firstTargetX); y--) {
            ptrdiff_t targetBaseY = y * TargetY;
            ptrdiff_t baseY = (kStartY + y) * BoardY;
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
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

    // Check order: down to up
    size_type partial_target_is_satisfy_reverse(const Board<BoardX, BoardY> & player,
                                                const Board<TargetX, TargetY> & target,
                                                size_type target_len,
                                                size_type firstTargetX, size_type lastTargetX,
                                                size_type firstTargetY, size_type lastTargetY) {
        for (size_type index = 0; index < target_len; index++) {
            if (this->partial_target_is_satisfy_reverse(player, target[index],
                                                        firstTargetX, lastTargetX,
                                                        firstTargetY, lastTargetY)) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
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

    size_type is_satisfy_phase1_1(const Board<BoardX, BoardY> & player,
                                  const Board<TargetX, TargetY> & target) {
        size_type mask = 0;

        // Left-Top Corner
        static const ptrdiff_t LeftTopX = kStartX;
        static const ptrdiff_t LeftTopY = kStartY;

        if (player.cells[LeftTopY * BoardY + LeftTopX] ==
            target.cells[0 * TargetY + 0]) {
            count_partial_color_nums_reverse(player, 0, LeftTopX + 1, 0, LeftTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 1;
            }
        }

        // Right-Top Corner
        static const ptrdiff_t RightTopX = kStartX + TargetX - 1;
        static const ptrdiff_t RightTopY = kStartY;

        if (player.cells[RightTopY * BoardY + RightTopX] ==
            target.cells[0 * TargetY + (TargetX - 1)]) {
            count_partial_color_nums_reverse(player, RightTopX, BoardX, 0, RightTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 2;
            }
        }

        // Left-Bottom Corner
        static const ptrdiff_t LeftBottomX = kStartX;
        static const ptrdiff_t LeftBottomY = kStartY + TargetY - 1;

        if (player.cells[LeftBottomY * BoardY + LeftBottomX] ==
            target.cells[(TargetY - 1) * TargetY + 0]) {
            count_partial_color_nums_reverse(player, 0, LeftBottomX + 1, LeftBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 4;
            }
        }

        // Right-Bottom Corner
        static const ptrdiff_t RightBottomX = kStartX + TargetX - 1;
        static const ptrdiff_t RightBottomY = kStartY + TargetY - 1;

        if (player.cells[RightBottomY * BoardY + RightBottomX] ==
            target.cells[(TargetY - 1) * TargetY + (TargetX - 1)]) {
            count_partial_color_nums_reverse(player, RightBottomX, BoardX, RightBottomY, BoardY);
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
            size_type mask = this->is_satisfy_phase1_1(player, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    size_type is_satisfy_phase1_12(const Board<BoardX, BoardY> & player,
                                   const Board<TargetX, TargetY> & target) {
        size_type mask = 0;

        // Left-Top Corner
        static const ptrdiff_t LeftTopX = kStartX;
        static const ptrdiff_t LeftTopY = kStartY;

        if (partial_target_is_satisfy(player, target, 0, 2, 0, 1)) {
            count_partial_color_nums_reverse(player, 0, LeftTopX + 2, 0, LeftTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 1;
            }
        }

        // Right-Top Corner
        static const ptrdiff_t RightTopX = kStartX + TargetX - 1;
        static const ptrdiff_t RightTopY = kStartY;

        if (partial_target_is_satisfy(player, target, TargetX - 2, TargetX, 0, 1)) {
            count_partial_color_nums_reverse(player, RightTopX - 1, BoardX, 0, RightTopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 2;
            }
        }

        // Left-Bottom Corner
        static const ptrdiff_t LeftBottomX = kStartX;
        static const ptrdiff_t LeftBottomY = kStartY + TargetY - 1;

        if (partial_target_is_satisfy(player, target, 0, 2, TargetY - 1, TargetY)) {
            count_partial_color_nums_reverse(player, 0, LeftBottomX + 2, LeftBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 4;
            }
        }

        // Right-Bottom Corner
        static const ptrdiff_t RightBottomX = kStartX + TargetX - 1;
        static const ptrdiff_t RightBottomY = kStartY + TargetY - 1;

        if (partial_target_is_satisfy(player, target, TargetX - 2, TargetX, TargetY - 1, TargetY)) {
            count_partial_color_nums_reverse(player, RightBottomX - 1, BoardX, RightBottomY, BoardY);
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
            size_type mask = this->is_satisfy_phase1_12(player, target[index]);
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
        static const ptrdiff_t TopY = kStartY;

        if (partial_target_is_satisfy(player, target, 0, TargetX, 0, 1)) {
            count_partial_color_nums_reverse(player, 0, BoardX, 0, TopY + 1);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 1;
            }
        }

        // Left partial
        static const ptrdiff_t LeftX = kStartX;

        if (partial_target_is_satisfy(player, target, 0, 1, 0, TargetY)) {
            count_partial_color_nums_reverse(player, 0, LeftX + 1, 0, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 2;
            }
        }

        // Right partial
        static const ptrdiff_t RightX = kStartX + TargetX - 1;

        if (partial_target_is_satisfy(player, target, TargetX - 1, TargetX, 0, TargetY)) {
            count_partial_color_nums_reverse(player, RightX, BoardX, 0, BoardY);
            if (this->partial_colors_[Color::Empty] == 0) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 4;
            }
        }

        // Bottom partial
        static const ptrdiff_t BottomY = kStartY + TargetY - 1;

        if (partial_target_is_satisfy(player, target, 0, TargetX, TargetY - 1, TargetY)) {
            count_partial_color_nums_reverse(player, 0, BoardX, BottomY, BoardY);
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
            size_type mask = this->is_satisfy_phase1_123(player, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    size_type is_satisfy_phase2_456(const Board<BoardX, BoardY> & player,
                                    const Board<TargetX, TargetY> & target) {
        size_type mask = 0;

        if (this->data_->phase2.phase1_type == 0) {
            // Top partial
            static const ptrdiff_t TopY = kStartY;

            if (partial_target_is_satisfy(player, target, 0, TargetX, 0, 2)) {
                count_partial_color_nums(player, 0, BoardX, TopY + 2, BoardY);
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 1;
            }
        }
        else if (this->data_->phase2.phase1_type == 1) {
            // Left partial
            static const ptrdiff_t LeftX = kStartX;

            if (partial_target_is_satisfy(player, target, 0, 2, 0, TargetY)) {
                count_partial_color_nums(player, LeftX + 2, BoardX, 0, BoardY);
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 2;
            }
        }
        else if (this->data_->phase2.phase1_type == 2) {
            // Right partial
            static const ptrdiff_t RightX = kStartX + TargetX - 1;

            if (partial_target_is_satisfy(player, target, TargetX - 2, TargetX, 0, TargetY)) {
                count_partial_color_nums(player, 0, kStartX + 1, 0, BoardY);
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 4;
            }
        }
        else if (this->data_->phase2.phase1_type == 3) {
            // Bottom partial
            static const ptrdiff_t BottomY = kStartY + TargetY - 1;

            if (partial_target_is_satisfy(player, target, 0, TargetX, TargetY - 2, TargetY)) {
                count_partial_color_nums(player, 0, BoardX, 0, kStartY + 1);
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

    size_type is_satisfy_phase2_456(const Board<BoardX, BoardY> & player,
                                    const Board<TargetX, TargetY> target[4],
                                    size_type target_len) {
        for (size_type index = 0; index < target_len; index++) {
            if (this->is_satisfy_phase2_456(player, target[index]) != 0) {
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
        if (this->partial_target_is_satisfy_reverse(player, target, 0, TargetX, 0, TargetY)) {
            mask |= 1;
        }

        size_u result(mask, 0);
        return result.value;
    }

    size_type is_satisfy_phase2(const Board<BoardX, BoardY> & player,
                                const Board<TargetX, TargetY> target[4],
                                size_type target_len) {
        for (size_type index = 0; index < target_len; index++) {
            size_type mask = this->is_satisfy_phase2(player, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

    bool is_satisfy_full(const Board<BoardX, BoardY> & player,
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

    size_type is_satisfy_full(const Board<BoardX, BoardY> & player,
                              const Board<TargetX, TargetY> target[4],
                              size_type target_len) const {
        for (size_type index = 0; index < target_len; index++) {
            if (this->is_satisfy_full(player, target[index])) {
                size_u result(1, index);
                return result.value;
            }
        }

        return 0;
    }

    size_type is_satisfy(const Board<BoardX, BoardY> & player,
                         const Board<TargetX, TargetY> & target) {
        if (N_SolverType == SolverType::Phase1_1) {
            return this->is_satisfy_phase1_1(player, target);
        }
        if (N_SolverType == SolverType::Phase1_12) {
            return this->is_satisfy_phase1_12(player, target);
        }
        else if (N_SolverType == SolverType::Phase1_123) {
            return this->is_satisfy_phase1_123(player, target);
        }
        else if (N_SolverType == SolverType::Phase2_456) {
            return this->is_satisfy_phase2_456(player, target);
        }
        else if (N_SolverType == SolverType::Phase2) {
            return this->is_satisfy_phase2(player, target);
        }
        else {
            return (size_type)this->is_satisfy_full(player, target);
        }

        return 0;
    }

    size_type is_satisfy(const Board<BoardX, BoardY> & player,
                         const Board<TargetX, TargetY> target[4],
                         size_type target_len) {
        if (AllowRotate) {
            if (N_SolverType == SolverType::Phase1_1) {
                return this->is_satisfy_phase1_1(player, target, target_len);
            }
            if (N_SolverType == SolverType::Phase1_12) {
                return this->is_satisfy_phase1_12(player, target, target_len);
            }
            else if (N_SolverType == SolverType::Phase1_123) {
                return this->is_satisfy_phase1_123(player, target, target_len);
            }
            else if (N_SolverType == SolverType::Phase2_456) {
                return this->is_satisfy_phase2_456(player, target, target_len);
            }
            else if (N_SolverType == SolverType::Phase2) {
                return this->is_satisfy_phase2(player, target, target_len);
            }
            else {
                return (size_t)this->is_satisfy_full(player, target, target_len);
            }
        }
        else {
            return this->is_satisfy(player, target[0]);
        }

        return 0;
    }

    size_type is_satisfy_all(const Board<BoardX, BoardY> & board,
                             const Board<TargetX, TargetY> target[4],
                             size_type target_len) {
        for (size_type index = 0; index < target_len; index++) {
            size_type mask = this->is_satisfy(board, target[index]);
            if (mask != 0) {
                size_u result(mask, index);
                return result.value;
            }
        }

        return 0;
    }

public:
    bool translateMovePath(const std::vector<Position> & move_path,
                           size_type rotate_type, Position empty_pos) {
        bool success = true;

        //printf("translateMovePath() begin ...\n\n");
        this->answer_.clear();

        Board<BoardX, BoardY> board(this->player_board_[rotate_type]);
        uint8_t from_pos, move_pos;
        uint8_t move_cell, from_cell;
        uint8_t last_dir = uint8_t(-1);
        assert(board.cells[empty_pos] == Color::Unknown);
        board.cells[empty_pos] = Color::Empty;
        bool found_empty = this->find_empty(board, empty_pos);
        if (!found_empty) {
            empty_pos = uint8_t(-1);
        }
        move_pos = empty_pos;
        for (size_type i = 0; i < move_path.size(); i++) {
            if (move_pos != uint8_t(-1))
                move_cell = board.cells[move_pos];
            else
                move_cell = Color::Unknown;
            assert_color(move_cell);
            from_pos = move_path[i].value;
            from_cell = board.cells[from_pos];
            assert_color(from_cell);
            if ((move_cell == Color::Empty || move_cell == Color::Unknown) &&
                (from_cell != Color::Empty)) {
                last_dir = Direction::template getDir<BoardX, BoardY>(from_pos, move_pos);
                MoveInfo move_info;
                move_info.from_pos = from_pos;
                move_info.move_pos = move_pos;
                move_info.color = from_cell;
                move_info.dir = last_dir;
                this->answer_.push_back(move_info);

                std::swap(board.cells[from_pos], board.cells[move_pos]);
            }
            else {
                printf("TargetBWSolver::translateMovePath():\n\n"
                        "Move path have error, [from_pos] is a empty gird.\n"
                        "index = %u, from_pos = %c%u, color = %s (%u)\n\n",
                        (uint32_t)(i + 1),
                        (uint32_t)Position::posToChr(from_pos / BoardY),
                        (uint32_t)(from_pos % BoardY) + 1,
                        Color::colorToChar(from_cell),
                        (uint32_t)from_cell);
                success = false;
                break;
            }
            move_pos = from_pos;
        }

        //printf("translateMovePath() end ...\n\n");
        return success;
    }

    bool translateMovePath(const stage_type & target_stage) {
        size_type n_rotate_type = target_stage.rotate_type;
        size_type empty_pos = n_rotate_type >> 2U;
        size_type rotate_type = n_rotate_type & 0x03U;
        return this->translateMovePath(target_stage.move_path, rotate_type, empty_pos);
    }

    void displayAnswer(const std::vector<MoveInfo> & answer) const {
        size_type index = 0;
        printf("Answer_Move_Path[%u] = {\n", (uint32_t)answer.size());
        for (auto iter : answer) {
            size_type from_pos  = iter.from_pos;
            size_type move_pos  = iter.move_pos;
            size_type color     = iter.color;
            size_type dir       = iter.dir;
            printf("    [%2u]: [%s], %c%u --> %c%u, dir: %-5s (%u)\n",
                   (uint32_t)(index + 1),
                   Color::colorToChar(color),
                   (uint32_t)Position::posToChr(from_pos / BoardY), (uint32_t)(from_pos % BoardY) + 1,
                   (uint32_t)Position::posToChr(move_pos / BoardY), (uint32_t)(move_pos % BoardY) + 1,
                   Direction::toString(dir),
                   (uint32_t)dir);
            index++;
        }
        printf("};\n\n");
    }

    void displayAnswer() const {
        return this->displayAnswer(this->answer_);
    }
};

} // namespace TwoEndpoint
} // namespace AI
} // namespace MagicBlock