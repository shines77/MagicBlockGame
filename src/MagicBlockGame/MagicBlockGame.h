#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <exception>
#include <stdexcept>

#include "Color.h"
#include "Move.h"
#include "SlidingPuzzle.h"

template <size_t BlocksX, size_t BlocksY, size_t TargetX, size_t TargetY>
class MagicBlockGame
{
public:
    static const size_t kTargetX = TargetX;
    static const size_t kTargetY = TargetY;
    static const size_t kBlocksX = BlocksX;
    static const size_t kBlocksY = BlocksY;

private:
    char target_[kTargetY][kTargetX];
    char blocks_[kBlocksY][kBlocksX];

    size_t              steps_;
    std::vector<Move>   moves_;

public:
    size_t getSteps() const {
        return this->steps_;
    }

    const std::vector<Move> & getMoves() const {
        return this->moves_;
    }

    int readInput(const char * filename) {
        int result = -1;
        int line_no = 0;
        std::ifstream ifs;
        try {
            ifs.open(filename, std::ios::in);
            if (ifs.good()) {
                result = 0;
                do { 
                    char line[256];
                    std::fill_n(line, sizeof(line), 0);
                    ifs.getline(line, 256);
                    if (line_no >= 0 && line_no < kTargetY) {
                        for (size_t x = 0; x < kTargetX; x++) {
                            char color = Color::valToColor(line[x]);
                            if (color >= Color::Red && color < Color::Maximum) {
                                this->target_[line_no][x] = color;
                            }
                            else {
                                result = -2;
                                break;
                            }
                        }
                    }
                    else if (line_no >= (kTargetY + 1) && line_no < (kTargetY + 1 + kBlocksY)) {
                        size_t blocksY = line_no - (kTargetY + 1);
                        for (size_t x = 0; x < kBlocksX; x++) {
                            char color = Color::valToColor(line[x]);
                            if (color >= Color::Empty && color < Color::Maximum) {
                                this->blocks_[blocksY][x] = color;
                            }
                            else {
                                result = -3;
                                break;
                            }
                        }
                    }
                    if (result < 0)
                        break;
                    line_no++;
                } while (!ifs.eof());

                ifs.close();

                if (result == 0)
                    result = 1;
            }
        }
        catch (std::exception & ex) {
            std::cout << "Exception: " << ex.what() << std::endl;
        }

        return result;
    }

    void translateMoves(const std::vector<Move> & moves) {
        //
    }

    bool solve() {
        //
        SlidingPuzzle<TargetX, TargetY> slidingPuzzle;
        slidingPuzzle.setPuzzle<BlocksX, BlocksY>(this->blocks_);
        bool success = slidingPuzzle.solve();
        if (success) {
            translateMoves(slidingPuzzle.getMoves());
        }
        return success;
    }
};
