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

    static const char * toString(size_t color) {
        switch (color) {
            case Color::Empty:
                return "Empty";
            case Color::Red:
                return "Red";
            case Color::Green:
                return "Green";
            case Color::Blue:
                return "Blue";
            case Color::White:
                return "White";
            case Color::Orange:
                return "Orange";
            case Color::Yellow:
                return "Yellow";
            case Color::Unknown:
                return "Unknown";
            default:
                return "Illegal";
        }
    }

    static const char * toShortString(size_t color) {
        switch (color) {
            case Color::Empty:
                return "E";
            case Color::Red:
                return "R";
            case Color::Green:
                return "G";
            case Color::Blue:
                return "B";
            case Color::White:
                return "W";
            case Color::Orange:
                return "O";
            case Color::Yellow:
                return "Y";
            case Color::Unknown:
                return "*";
            default:
                return "?";
        }
    }
};

} // namespace PuzzleGame
