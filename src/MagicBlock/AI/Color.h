#pragma once

#include <stdint.h>
#include <stddef.h>

namespace MagicBlock {
namespace AI {

struct Color {
    enum {
        Empty,              // 0
        First,              // 1
        Red = First,        // 1
        Green,              // 2
        Blue,               // 3
        White,              // 4
        Orange,             // 5
        Yellow,             // 6
        Unknown,            // 7
        Last = Unknown,     // 7
        Illegal,            // 8
        Maximum = Illegal   // 8
    };

    static const std::size_t   Mask    = 0x00000007ULL;
    static const std::size_t   Shift   = 3;
    static const std::uint32_t Mask32  = 0x00000007UL;
    static const std::uint32_t Shift32 = 3;

    static uint8_t charToColor(uint8_t value) {
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
        case '*':
            return Color::Unknown;
        default:
            return Color::Illegal;
        }
    }

    static const char * colorToChar(size_t color) {
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
};

} // namespace AI
} // namespace MagicBlock