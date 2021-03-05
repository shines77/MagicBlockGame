#pragma once

#include <stdint.h>
#include <stddef.h>

#include <vector>

#include "Move.h"
#include "Board.h"

namespace PuzzleGame {

#pragma pack(push, 1)

template <size_t BoardX, size_t BoardY>
struct Stage {
    Position    empty;
    uint8_t     last_dir, reserve;
    Board<BoardX, BoardY> board;
    std::vector<Move> move_path;

    Stage() {}
    Stage(const Board<BoardX, BoardY> & srcBoard) {
        this->board = srcBoard;
    }
};

#pragma pack(pop)

} // namespace PuzzleGame
