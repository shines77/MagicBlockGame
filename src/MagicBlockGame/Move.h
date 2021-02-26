#pragma once

#include <stdint.h>
#include <stddef.h>

struct Direction {
    enum Dir {
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

struct Move {
    Position pos;
    uint8_t dir;
    uint8_t reserve;
};
