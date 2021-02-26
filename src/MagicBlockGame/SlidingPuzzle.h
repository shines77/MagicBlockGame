#pragma once

#include "Color.h"
#include "Move.h"

template <size_t CellsX, size_t CellsY>
class SlidingPuzzle
{
private:
    char cells_[CellsY][CellsX];
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
        ptrdiff_t startX = (BlocksX - CellsX) / 2;
        ptrdiff_t startY = (BlocksY - CellsY) / 2;
        for (size_t y = 0; y < CellsY; y++) {
            for (size_t x = 0; x < CellsX; x++) {
                this->cells_[y][x] = blocks[startY + y][startX + x];
            }
        }
    }

    bool solve() {
        bool solvable = true;
        return solvable;
    }
};
