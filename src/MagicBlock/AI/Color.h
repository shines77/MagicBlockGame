#pragma once

#include <stdint.h>
#include <stddef.h>

namespace MagicBlock {
namespace AI {

struct Color {
    enum {
        First   = 0,        // 0
        Red     = 0,        // 0
        Green   = 1,        // 1
        Blue    = 2,        // 2
        White   = 3,        // 3
        Orange  = 4,        // 4
        Yellow  = 5,        // 5
        Empty   = 6,        // 6
        Unknown = 7,        // 7
        Mask    = 7,        // 7
        Last    = Unknown,  // 7
        Illegal = 8,        // 8
        Maximum = Illegal   // 8
    };

    static const std::size_t   Mask    = 0x00000007ULL;
    static const std::size_t   Shift   = 3;
    static const std::uint32_t Mask32  = 0x00000007UL;
    static const std::uint32_t Shift32 = 3;

    static uint8_t charToColor(uint8_t value) {
        switch (value) {
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
        case ' ':
        case 'E':
            return Color::Empty;
        case '*':
            return Color::Unknown;
        default:
            return Color::Illegal;
        }
    }

    static const char * colorToChar(size_t color) {
        switch (color) {
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
            case Color::Empty:
                return "E";
            case Color::Unknown:
                return "*";
            default:
                return "?";
        }
    }

    static const char * toString(size_t color) {
        switch (color) {
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
            case Color::Empty:
                return "Empty";
            case Color::Unknown:
                return "Unknown";
            default:
                return "Illegal";
        }
    }
};

} // namespace AI
} // namespace MagicBlock
