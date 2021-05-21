#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <array>

#include "MagicBlock/AI/Move.h"

namespace MagicBlock {
namespace AI {

template <std::size_t BoardX, std::size_t BoardY>
class CanMoves {
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

    static const size_type X = BoardX;
    static const size_type Y = BoardY;
    static const size_type BoardSize = BoardX * BoardY;

    class CanMoveList {
    private:
        size_type size_;
        std::array<Move, Dir::Maximum> list_;

        void internal_copy(const CanMoveList & other) noexcept {
            this->size_ = other.size_;
            this->list_ = other.list_;
        }

        void internal_move(CanMoveList && other) noexcept {
            this->size_ = other.size_;
            this->list_ = std::move(other.list_);
        }

        void internal_swap(CanMoveList & other) noexcept {
            std::swap(this->size_, other.size_);
            std::swap(this->list_, other.list_);
        }

    public:
        CanMoveList() noexcept : size_(0) {}
        CanMoveList(const CanMoveList & src) noexcept : size_(src.size_), list_(src.list_) {
        }
        CanMoveList(CanMoveList && src) noexcept {
            this->internal_move(std::forward<CanMoveList>(src));
        }
        ~CanMoveList() {}

        CanMoveList & operator = (const CanMoveList & rhs) noexcept {
            this->copy(rhs);
            return *this;
        }

        CanMoveList & operator = (CanMoveList && rhs) noexcept {
            this->move(rhs);
            return *this;
        }

        size_type size() const noexcept {
            return this->size_;
        }

        size_type capacity() const noexcept {
            return Dir::Maximum;
        }

        bool empty() const noexcept {
            return (this->size_ == 0);
        }

        Move * data() noexcept {
            return this->list_.data();
        }

        const Move * data() const noexcept {
            return this->list_.data();
        }

        void set_size(size_type size) noexcept {
            this->size_ = size;
        }

        Move & operator [] (size_type pos) noexcept {
            assert(pos < this->capacity());
            return this->list_[pos];
        }

        const Move & operator [] (size_type pos) const noexcept {
            assert(pos < this->capacity());
            return this->list_[pos];
        }

        void copy(const CanMoveList & other) noexcept {
            if (&other != this) {
                this->internal_copy(other);
            }
        }

        void move(CanMoveList && other) noexcept {
            if (&other != this) {
                this->internal_move(std::forward<CanMoveList>(other));
            }
        }

        void swap(CanMoveList & other) noexcept {
            if (&other != this) {
                this->internal_swap(other);
            }
        }
    };

    typedef CanMoveList                             can_move_list_t;
    typedef std::array<can_move_list_t, BoardSize>  board_can_moves_t;

    typedef CanMoves<BoardX, BoardY>                this_type;

private:
    board_can_moves_t array_;

    void internal_copy(const board_can_moves_t & other) {
        for (size_type pos = 0; pos < BoardSize; pos++) {
            this->array_[pos] = other.array_[pos];
        }
    }

    void internal_move(board_can_moves_t && other) {
        for (size_type pos = 0; pos < BoardSize; pos++) {
            this->array_[pos] = std::move(other.array_[pos]);
        }
    }

    void internal_swap(board_can_moves_t & other) {
        for (size_type pos = 0; pos < BoardSize; pos++) {
            this->array_[pos].swap(other.array_[pos]);
        }
    }

public:
    CanMoves() noexcept {}
    CanMoves(const CanMoves & src) noexcept {
        internal_copy(src);
    }
    CanMoves(CanMoves && src) noexcept {
        internal_move(src);
    }
    ~CanMoves() {}

    CanMoves & operator = (const CanMoves & rhs) noexcept {
        this->copy(rhs);
        return *this;
    }

    CanMoves & operator = (CanMoves && rhs) noexcept {
        this->move(rhs);
        return *this;
    }

    size_type size() const noexcept {
        return this->array_.size();
    }

    size_type capacity() const noexcept {
        return BoardSize;
    }

    bool empty() const noexcept {
        return this->array_.empty();
    }

    can_move_list_t * data() noexcept {
        return this->array_.data();
    }

    const can_move_list_t * data() const noexcept {
        return this->array_.data();
    }

    can_move_list_t & operator [] (size_type pos) noexcept {
        assert(pos < this->capacity());
        return this->array_[pos];
    }

    const can_move_list_t & operator [] (size_type pos) const noexcept {
        assert(pos < this->capacity());
        return this->array_[pos];
    }

    void copy(const CanMoves & other) noexcept {
        if (&other != this) {
            this->internal_copy(other);
        }
    }

    void move(CanMoves && other) noexcept {
        if (&other != this) {
            this->internal_move(std::forward<CanMoves>(other));
        }
    }

    void swap(CanMoves & other) noexcept {
        if (&other != this) {
            this->internal_swap(other);
        }
    }

    static void makeUp(CanMoves & can_moves) {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                size_type pos = y * BoardX + x;
                assert(pos < BoardSize);
                size_type dir_count = 0;
                for (size_type dir = 0; dir < Dir::Maximum; dir++) {
                    assert(dir >= 0 && dir < 4);
                    int board_x = (int)x + Dir_Offset[dir].x;
                    if (board_x < 0 || board_x >= (int)BoardX)
                        continue;
                    int board_y = (int)y + Dir_Offset[dir].y;
                    if (board_y < 0 || board_y >= (int)BoardY)
                        continue;
                    Move move;
                    move.pos = Position(board_y * (int)BoardX + board_x);
                    move.dir = Dir::opp_dir(dir);
                    can_moves[pos][dir_count] = move;
                    dir_count++;
                }
                can_moves[pos].set_size(dir_count);
            }
        }
    }

    static CanMoves & getInstance() {
        static CanMoves can_moves;
        static bool inited = false;
        if (!inited) {
            this_type::makeUp(can_moves);
            inited = true;
        }
        return can_moves;
    }

    static void copyTo(CanMoves & dest, CanMoves & src) {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                size_type pos = y * BoardX + x;
                assert(pos < BoardSize);
                dest[pos] = src[pos];
            }
        }
    }

    static void copyTo(CanMoves & in_can_moves) {
        CanMoves & can_moves = this_type::getInstance();
        this_type::copyTo(in_can_moves, can_moves);
    }
};

} // namespace AI
} // namespace MagicBlock
