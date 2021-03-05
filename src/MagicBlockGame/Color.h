#pragma once

#include <stdint.h>
#include <stddef.h>

namespace PuzzleGame {

struct Color {
    enum {
        Empty,
        First,
        Red = First,
        Green,
        Blue,
        White,
        Orange,
        Yellow,
        Last,
        Unknown = Last,
        Maximum
    };

    static uint8_t valToColor(uint8_t value) {
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
};

} // namespace PuzzleGame
