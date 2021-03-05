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

#pragma pack(push, 1)

struct Position8 {
    int8_t x;
    int8_t y;

    Position8() : x(0), y(0) {}
    Position8(int _x, int _y) : x(_x), y(_y) {}
};

struct Position {
    int16_t value;

    Position() : value(0) {}
    Position(int _value) : value(_value) {}
};

struct Move {
    Position    pos;
    uint8_t     dir;
    uint8_t     reserve;
};

#pragma pack(pop)

} // namespace PuzzleGame
