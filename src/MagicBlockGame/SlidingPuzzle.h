#pragma once

#include "Color.h"
#include "Move.h"

template <size_t CellsX, size_t CellsY>
class SlidingPuzzle
{
public:
    static const size_t kCellsX = CellsX;
    static const size_t kCellsY = CellsY;

private:
    char cells_[kCellsY][kCellsX];
    size_t              steps_;
    std::vector<Move>   moves_;

public:
    SlidingPuzzle() {}
    ~SlidingPuzzle() {}

    size_t getSteps() const {
        return this->steps_;
    }

    const std::vector<Move> & getMoves() const {
        return this->moves_;
    }

    template <size_t BlocksX, size_t BlocksY>
    void setPuzzle(char blocks[BlocksY][BlocksX]) {
        ptrdiff_t startX = (BlocksX - kCellsX) / 2;
        ptrdiff_t startY = (BlocksY - kCellsY) / 2;
        for (size_t y = 0; y < kCellsY; y++) {
            for (size_t x = 0; x < kCellsX; x++) {
                this->cells_[y][x] = blocks[startY + y][startX + x];
            }
        }
    }

    bool solve() {
        //
        return true;
    }
};
