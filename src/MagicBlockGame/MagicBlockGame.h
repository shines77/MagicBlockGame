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
#include "Board.h"
#include "SlidingPuzzle.h"

namespace PuzzleGame {

template <size_t BoardX, size_t BoardY, size_t TargetX, size_t TargetY>
class MagicBlockGame
{
private:
    Board<BoardX, BoardY> board_;
    Board<TargetX, TargetY> target_;

    std::vector<Move> moves_;

public:
    size_t getSteps() const {
        return this->moves_.size();
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
                    if (line_no >= 0 && line_no < TargetY) {
                        for (size_t x = 0; x < TargetX; x++) {
                            char color = Color::valToColor(line[x]);
                            if (color >= Color::Empty && color < Color::Maximum) {
                                this->target_.cells[line_no * TargetY + x] = color;
                            }
                            else {
                                result = -2;
                                break;
                            }
                        }
                    }
                    else if (line_no >= (TargetY + 1) && line_no < (TargetY + 1 + BoardY)) {
                        size_t boardY = line_no - (TargetY + 1);
                        for (size_t x = 0; x < BoardX; x++) {
                            char color = Color::valToColor(line[x]);
                            if (color >= Color::Empty && color < Color::Maximum) {
                                this->board_.cells[boardY * BoardY + x] = color;
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

                if (result == 0) {
                    result = 1;
                }
                else {
                    char err_info[256] = {0};
                    snprintf(err_info, sizeof(err_info) - 1, "MagicBlockGame::readInput() Error code = %d", result);
                    throw std::runtime_error(err_info);
                }
            }
        }
        catch (std::exception & ex) {
            std::cout << "Exception: " << ex.what() << std::endl << std::endl;
        }

        return result;
    }

    void translateMoves(const std::vector<Move> & moves) {
        this->moves_ = moves;
    }

    bool solve() {
        SlidingPuzzle<TargetX, TargetY> slidingPuzzle;
        slidingPuzzle.setPuzzle<BoardX, BoardY>(this->board_, this->target_);
        bool solvable = slidingPuzzle.solve();
        if (solvable) {
            translateMoves(slidingPuzzle.getMoves());
        }
        return solvable;
    }
};

} // namespace PuzzleGame
