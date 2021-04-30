#pragma once

#include <stdint.h>
#include <stddef.h>

#include <vector>

#include "Move.h"
#include "Board.h"

namespace MagicBlock {

#pragma pack(push, 1)

template <size_t BoardX, size_t BoardY>
struct Stage {
    Board<BoardX, BoardY> board;

    Position    empty;
    uint8_t     last_dir, rotate_type;
    uint8_t     reserve;

    std::vector<Position> move_path;

    Stage() : empty(0), last_dir(0), rotate_type(0), reserve(0) {}
    Stage(const Board<BoardX, BoardY> & board) {
        this->board = board;
    }
};

#pragma pack(pop)

} // namespace MagicBlock
