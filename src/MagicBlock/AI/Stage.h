#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstdint>
#include <cstddef>
#include <vector>

#include "MagicBlock/AI/Move.h"
#include "MagicBlock/AI/Board.h"

namespace MagicBlock {
namespace AI {

#pragma pack(push, 1)

template <std::size_t BoardX, std::size_t BoardY>
struct Stage {
    Board<BoardX, BoardY> board;

    Position    empty_pos;
    uint8_t     last_dir, rotate_type;
    uint8_t     reserve;

    std::vector<Position> move_path;

    Stage() noexcept : empty_pos(0), last_dir(0), rotate_type(0), reserve(0) {}
    Stage(const Stage & src) noexcept {
        this->internal_copy(src);
    }
    Stage(Stage && src) noexcept {
        this->internal_swap(src);
    }
    Stage(const Board<BoardX, BoardY> & _board) noexcept : board(_board),
        empty_pos(0), last_dir(0), rotate_type(0), reserve(0) {
    }

    ~Stage() {}

    Stage & operator = (const Stage & rhs) noexcept {
        this->copy(rhs);
        return *this;
    }

    Stage & operator = (Stage && rhs) noexcept {
        this->swap(rhs);
        return *this;
    }

    void internal_copy(const Stage & other) noexcept {
        this->board         = other.board;

        this->empty_pos     = other.empty_pos;
        this->last_dir      = other.last_dir;
        this->rotate_type   = other.rotate_type;
        this->reserve       = other.reserve;

        this->move_path     = other.move_path;
    }

    void copy(const Stage & other) noexcept {
        if (&other != this) {
            this->internal_copy(other);
        }
    }

    void internal_swap(Stage & other) noexcept {
        this->board.swap(other.board);

        this->empty_pos.swap(other.empty_pos);
        std::swap(this->last_dir, other.last_dir);
        std::swap(this->rotate_type, other.rotate_type);
        std::swap(this->reserve, other.reserve);

        std::swap(this->move_path, other.move_path);
    }

    void swap(Stage & other) noexcept {
        if (&other != this) {
            this->internal_swap(other);
        }
    }
};

template <std::size_t BoardX, std::size_t BoardY>
inline
void swap(Stage<BoardX, BoardY> & lhs, Stage<BoardX, BoardY> & rhs) noexcept {
    lhs.swap(rhs);
}

template <std::size_t BoardX, std::size_t BoardY>
struct StageInfo {
    std::size_t rotate_type;
    std::size_t phase1_type;

    Stage<BoardX, BoardY> stage;

    StageInfo() noexcept : rotate_type(0), phase1_type(0)  {
    }

    StageInfo(std::size_t _rotate_type, std::size_t _phase1_type) noexcept
        : rotate_type(_rotate_type), phase1_type(_phase1_type) {
    }

    ~StageInfo() {
    }
};

#pragma pack(pop)

} // namespace AI
} // namespace MagicBlock
