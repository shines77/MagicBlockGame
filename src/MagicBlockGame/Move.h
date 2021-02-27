#pragma once

#include <stdint.h>
#include <stddef.h>

namespace PuzzleGame {

struct Direction {
    enum {
        First,
        Up = First,
        Right,
        Down,
        Left,
        Last
    };
};

struct Offset {
    int x;
    int y;
};
    
static const Offset Dir_Offset[Direction::Last] = {
    {  0,  1 },
    {  1,  0 },
    {  0, -1 },
    { -1,  0 }
};

struct Position {
    int8_t x;
    int8_t y;

    Position() : x(0), y(0) {}
    Position(int _x, int _y) : x(_x), y(_y) {}
};

struct Position16 {
    int16_t value;

    Position16() : value(0) {}
    Position16(int _value) : value(_value) {}
};

struct Move {
    Position16  pos;
    uint8_t     dir;
    uint8_t     reserve;
};

} // namespace PuzzleGame
