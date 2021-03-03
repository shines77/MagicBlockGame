#pragma once

#include <stdint.h>
#include <stddef.h>

#include <vector>

#include "Move.h"
#include "Board.h"

namespace PuzzleGame {

template <size_t BoardX, size_t BoardY>
struct Stage {
    Position16  empty;
    uint8_t     last_dir, reserve;
    Board<BoardX, BoardY> board;
    std::vector<Move> moves;

    Stage() {}
    Stage(const Board<BoardX, BoardY> & srcBoard) {
        this->board = srcBoard;
    }
};

} // namespace PuzzleGame
