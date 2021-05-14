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
class BaseBWSolver
{
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

    typedef BaseBWSolver<BoardX, BoardY, TargetX, TargetY, AllowRotate, N_SolverType, Phase2CallBack>  this_type;

    typedef SharedData<BoardX, BoardY, TargetX, TargetY>    shared_data_type;
    typedef typename shared_data_type::stage_type           stage_type;
    typedef typename shared_data_type::player_board_t       player_board_t;
    typedef typename shared_data_type::target_board_t       target_board_t;
    typedef Phase2CallBack                                  phase2_callback;

    static const size_type BoardSize = BoardX * BoardY;
    static const size_type kSingelColorNums = (BoardSize - 1) / (Color::Last - 1);

    static const ptrdiff_t kStartX = (BoardX - TargetX) / 2;
    static const ptrdiff_t kStartY = (BoardY - TargetY) / 2;

protected:
    shared_data_type * data_;

    Board<BoardX, BoardY>   player_board_[4];
    Board<BoardX, BoardX>   target_board_;
    Board<TargetX, TargetX> fw_target_board_[4];

    size_type target_len_;
    size_type rotate_type_;

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
    BaseBWSolver(shared_data_type * data)
        : data_(data), target_len_(0), rotate_type_(0), map_used_(0) {
        this->init();
    }

    ~BaseBWSolver() {
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
        assert(color >= Color::First && color < Color::Maximum);
    }

    void setRotateType(size_type rotate_type) {
        assert((rotate_type >= 0 && rotate_type < MAX_ROTATE_TYPE) || (rotate_type == size_type(-1)));
        this->rotate_type_ = rotate_type;
    }

    void setPlayerBoardFromTargetBoard(Board<BoardX, BoardY> & player_board,
                                       const Board<TargetX, TargetY> & target_board) {
        // Fill the Color::Unknown to player board
        for (size_type pos = 0; pos < BoardSize; pos++) {
            player_board.cells[pos] = Color::Unknown;
        }

        // Copy input target board to player board
        for (size_type y = 0; y < TargetY; y++) {
            for (size_type x = 0; x < TargetX; x++) {
                uint8_t clr = target_board.cells[y * TargetX + x];
                assert_color(clr);
                size_type new_pos = (y + kStartY) * BoardX + (x + kStartX);
                player_board.cells[new_pos] = clr;
            }
        }
    }

    bool find_empty(const Board<BoardX, BoardY> & board, Position & empty_pos) const {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t clr = board.cells[y * BoardX + x];
                if (clr == Color::Empty) {
                    empty_pos = (uint8_t)(y * BoardX + x);
                    return true;
                }
            }
        }
        return false;
    }

    bool find_unknown(const Board<BoardX, BoardY> & board,
                      size_type start_pos, Position & unknown_pos) const {
        for (size_type pos = start_pos; pos < BoardSize; pos++) {
            uint8_t clr = board.cells[pos];
            if (clr == Color::Unknown) {
                unknown_pos = pos;
                return true;
            }
        }
        return false;
    }

    void find_all_colors(const Board<BoardX, BoardY> & board,
                         size_type color, std::vector<Position> & pos_list) {
        for (size_type pos = 0; pos < BoardSize; pos++) {
            uint8_t clr = board.cells[pos];
            if (clr == color) {
                pos_list.push_back(pos);
            }
        }
    }

    void count_partial_color_nums(const Board<BoardX, BoardY> & board,
                                  size_type firstX, size_type lastX,
                                  size_type firstY, size_type lastY) {
        for (size_type clr = Color::First; clr < Color::Maximum; clr++) {
            this->partial_colors_[clr] = 0;
        }

        for (size_type y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardX;
            for (size_type x = firstX; x < lastX; x++) {
                uint8_t clr = board.cells[baseY + x];
                assert_color(clr);
                this->partial_colors_[clr]++;
            }
        }
    }

    void count_partial_color_nums_reverse(const Board<BoardX, BoardY> & board,
                                          size_type firstX, size_type lastX,
                                          size_type firstY, size_type lastY) {
        for (size_type clr = Color::First; clr < Color::Maximum; clr++) {
            this->partial_colors_[clr] = kSingelColorNums;
        }
        this->partial_colors_[Color::Empty] = 1;

        for (size_type y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardX;
            for (size_type x = firstX; x < lastX; x++) {
                uint8_t clr = board.cells[baseY + x];
                assert_color(clr);
                this->partial_colors_[clr]--;
            }
        }
    }

    void count_target_color_nums(const Board<TargetX, TargetY> & target) {
        for (size_type clr = Color::First; clr < Color::Maximum; clr++) {
            this->target_colors_[clr] = 0;
        }

        for (size_type y = 0; y < TargetY; y++) {
            for (size_type x = 0; x < TargetX; x++) {
                uint8_t clr = target.cells[y * TargetX + x];
                assert_color(clr);
                this->target_colors_[clr]++;
            }
        }
    }

    void count_partial_target_color_nums(const Board<TargetX, TargetY> & target,
                                         size_type firstTargetX, size_type lastTargetX,
                                         size_type firstTargetY, size_type lastTargetY) {
        for (size_type clr = Color::First; clr < Color::Maximum; clr++) {
            this->target_colors_[clr] = 0;
        }

        for (size_type y = firstTargetY; y < lastTargetY; y++) {
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
                uint8_t clr = target.cells[y * TargetX + x];
                assert_color(clr);
                this->target_colors_[clr]++;
            }
        }
    }

    void count_target_color_nums(const Board<BoardX, BoardY> & target) {
        for (size_type clr = Color::First; clr < Color::Maximum; clr++) {
            this->target_colors_[clr] = 0;
        }

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t clr = target.cells[y * BoardX + x];
                assert_color(clr);
                this->target_colors_[clr]++;
            }
        }
    }

    void count_partial_target_color_nums(const Board<BoardX, BoardY> & target,
                                         size_type firstTargetX, size_type lastTargetX,
                                         size_type firstTargetY, size_type lastTargetY) {
        for (size_type clr = Color::First; clr < Color::Maximum; clr++) {
            this->target_colors_[clr] = 0;
        }

        for (size_type y = firstTargetY; y < lastTargetY; y++) {
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
                uint8_t clr = target.cells[y * BoardX + x];
                assert_color(clr);
                this->target_colors_[clr]++;
            }
        }
    }

    bool check_partial_color_nums() const {
        if (this->is_phase2()) {
            for (size_type clr = Color::First; clr < Color::Maximum; clr++) {
                if (this->partial_colors_[clr] < this->target_colors_[clr]) {
                    return false;
                }
            }
        }
        else {
            for (size_type clr = Color::First; clr < Color::Maximum; clr++) {
                if (this->partial_colors_[clr] < this->target_colors_[clr]) {
                    return false;
                }
            }
        }
        return true;
    }

    // Check order: up to down
    bool partial_target_is_satisfy(const Board<BoardX, BoardY> & player,
                                   const Board<TargetX, TargetY> & target,
                                   size_type firstTargetX, size_type lastTargetX,
                                   size_type firstTargetY, size_type lastTargetY) {
        for (size_type y = firstTargetY; y < lastTargetY; y++) {
            ptrdiff_t targetBaseY = y * TargetX;
            ptrdiff_t playerBaseY = (kStartY + y) * BoardX;
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
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
            ptrdiff_t targetBaseY = y * TargetX;
            ptrdiff_t playerBaseY = (kStartY + y) * BoardX;
            for (size_type x = firstTargetX; x < lastTargetX; x++) {
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

    void locked_partial_board(int locked[BoardSize],
                              size_type firstX, size_type lastX,
                              size_type firstY, size_type lastY) {
        for (size_type y = 0; y < BoardY; y++) {
            ptrdiff_t baseY = y * BoardX;
            for (size_type x = 0; x < BoardX; x++) {
                locked[baseY + x] = 0;
            }
        }

        for (size_type y = firstY; y < lastY; y++) {
            ptrdiff_t baseY = y * BoardX;
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

        if (player.cells[LeftTopY * BoardX + LeftTopX] ==
            target.cells[0 * TargetX + 0]) {
            count_partial_color_nums_reverse(player, 0, LeftTopX + 1, 0, LeftTopY + 1);
            if (this->partial_colors_[Color::Empty] == 1) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 1;
            }
        }

        // Right-Top Corner
        static const ptrdiff_t RightTopX = kStartX + TargetX - 1;
        static const ptrdiff_t RightTopY = kStartY;

        if (player.cells[RightTopY * BoardX + RightTopX] ==
            target.cells[0 * TargetX + (TargetX - 1)]) {
            count_partial_color_nums_reverse(player, RightTopX, BoardX, 0, RightTopY + 1);
            if (this->partial_colors_[Color::Empty] == 1) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 2;
            }
        }

        // Left-Bottom Corner
        static const ptrdiff_t LeftBottomX = kStartX;
        static const ptrdiff_t LeftBottomY = kStartY + TargetY - 1;

        if (player.cells[LeftBottomY * BoardX + LeftBottomX] ==
            target.cells[(TargetY - 1) * TargetX + 0]) {
            count_partial_color_nums_reverse(player, 0, LeftBottomX + 1, LeftBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 1) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 4;
            }
        }

        // Right-Bottom Corner
        static const ptrdiff_t RightBottomX = kStartX + TargetX - 1;
        static const ptrdiff_t RightBottomY = kStartY + TargetY - 1;

        if (player.cells[RightBottomY * BoardX + RightBottomX] ==
            target.cells[(TargetY - 1) * TargetX + (TargetX - 1)]) {
            count_partial_color_nums_reverse(player, RightBottomX, BoardX, RightBottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 1) {
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
            if (this->partial_colors_[Color::Empty] == 1) {
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
            if (this->partial_colors_[Color::Empty] == 1) {
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
            if (this->partial_colors_[Color::Empty] == 1) {
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
            if (this->partial_colors_[Color::Empty] == 1) {
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
            if (this->partial_colors_[Color::Empty] == 1) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 1;
            }
        }

        // Left partial
        static const ptrdiff_t LeftX = kStartX;

        if (partial_target_is_satisfy(player, target, 0, 1, 0, TargetY)) {
            count_partial_color_nums_reverse(player, 0, LeftX + 1, 0, BoardY);
            if (this->partial_colors_[Color::Empty] == 1) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 2;
            }
        }

        // Right partial
        static const ptrdiff_t RightX = kStartX + TargetX - 1;

        if (partial_target_is_satisfy(player, target, TargetX - 1, TargetX, 0, TargetY)) {
            count_partial_color_nums_reverse(player, RightX, BoardX, 0, BoardY);
            if (this->partial_colors_[Color::Empty] == 1) {
                bool is_valid = check_partial_color_nums();
                if (is_valid)
                    mask |= 4;
            }
        }

        // Bottom partial
        static const ptrdiff_t BottomY = kStartY + TargetY - 1;

        if (partial_target_is_satisfy(player, target, 0, TargetX, TargetY - 1, TargetY)) {
            count_partial_color_nums_reverse(player, 0, BoardX, BottomY, BoardY);
            if (this->partial_colors_[Color::Empty] == 1) {
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
            ptrdiff_t targetBaseY = y * TargetX;
            ptrdiff_t playerBaseY = (kStartY + y) * BoardX;
            for (size_type x = 0; x < TargetX; x++) {
                uint8_t target_cell = target.cells[targetBaseY + x];
                uint8_t cell = player.cells[playerBaseY + (kStartX + x)];
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
                           size_type rotate_type, Position empty_pos,
                           std::vector<MoveInfo> & answer) const {
        assert(rotate_type >= 0 && rotate_type < MAX_ROTATE_TYPE);
        return this->player_board_[rotate_type].translate_move_path(move_path, answer, empty_pos);
    }

    bool translateMovePath(const stage_type & target_stage) {
        size_type n_rotate_type = target_stage.rotate_type;
        size_type empty_pos = n_rotate_type >> 2U;
        size_type rotate_type = n_rotate_type & 0x03U;
        return this->translateMovePath(target_stage.move_path, rotate_type, empty_pos, this->answer_);
    }

    bool translateMovePath(const stage_type & target_stage, std::vector<MoveInfo> & answer) const {
        size_type n_rotate_type = target_stage.rotate_type;
        size_type empty_pos = n_rotate_type >> 2U;
        size_type rotate_type = n_rotate_type & 0x03U;
        return this->translateMovePath(target_stage.move_path, rotate_type, empty_pos, answer);
    }

    void displayAnswer(const std::vector<MoveInfo> & answer) const {
        player_board_t::template display_answer<BoardX>(answer);
    }

    void displayAnswer() const {
        return this->displayAnswer(this->answer_);
    }

    void displayAnswer(const std::vector<Position> & move_path,
                       size_type rotate_type, Position empty_pos) const {
        std::vector<MoveInfo> answer;
        if (this->translateMovePath(move_path, rotate_type, empty_pos, answer)) {
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

} // namespace TwoEndpoint
} // namespace AI
} // namespace MagicBlock
