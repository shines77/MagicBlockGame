
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstring>
#include <exception>
#include <stdexcept>

struct Color {
    enum : char {
        Empty,
        Red,
        Green,
        Blue,
        White,
        Orange,
        Yellow,
        Unknown,
        Maximum = Unknown
    };
};

class MagicBlockGame
{
public:
    static const size_t kTargetX = 3;
    static const size_t kTargetY = 3;
    static const size_t kBlocksX = 5;
    static const size_t kBlocksY = 5;

private:
    char target_[kTargetX][kTargetY];
    char blocks_[kBlocksX][kBlocksY];

public:
    char valToColor(char value) {
        switch (value) {
        case ' ':
        case 'E':
            return Color::Empty;
        case 'R':
            return Color::Red;
        case 'G':
            return Color::Green;
        case 'B':
            return Color::Blue;
        case 'W':
            return Color::White;
        case 'O':
            return Color::Orange;
        case 'Y':
            return Color::Yellow;
        default:
            return Color::Unknown;
        }
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
                        for (size_t i = 0; i < kTargetX; i++) {
                            char color = valToColor(line[i]);
                            if (color >= Color::Red && color < Color::Maximum) {
                                this->target_[line_no][i] = color;
                            }
                            else {
                                result = -2;
                                break;
                            }
                        }
                    }
                    else if (line_no >= (kTargetY + 1) && line_no < (kTargetY + 1 + kBlocksY)) {
                        size_t blocksY = line_no - (kTargetY + 1);
                        for (size_t i = 0; i < kBlocksX; i++) {
                            char color = valToColor(line[i]);
                            if (color >= Color::Empty && color < Color::Maximum) {
                                this->blocks_[blocksY][i] = color;
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

    int solve() {
        //
        return 0;
    }
};
