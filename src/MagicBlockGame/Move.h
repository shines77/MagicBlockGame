#pragma once

#include <stdint.h>
#include <stddef.h>

struct Direction {
    enum {
        Up,
        Right,
        Down,
        Left,
        Maximum
    };
};

struct Move {
    uint8_t x;
    uint8_t y;
    uint8_t dir;
    uint8_t reserve;
};
