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
    Position    empty;
    uint8_t     last_dir, rotate_type;

    Board<BoardX, BoardY> board;
    std::vector<Position> move_path;

    Stage() {}
    Stage(const Board<BoardX, BoardY> & board) {
        this->board = board;
    }
};

#pragma pack(pop)

} // namespace PuzzleGame
