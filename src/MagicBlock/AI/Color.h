#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstdint>
#include <cstddef>

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
        Last    = Unknown,  // 7
        Illegal = 8,        // 8
        Maximum = Illegal   // 8
    };

    static const std::size_t   Mask    = 0x00000007ULL;
    static const std::size_t   Shift   = 3;
    static const std::uint32_t Mask32  = 0x00000007UL;
    static const std::uint32_t Shift32 = 3;

    static std::uint8_t toColor(std::uint8_t ascii) {
        switch (ascii) {
        case 'R':
            return (std::uint8_t)Color::Red;
        case 'G':
            return (std::uint8_t)Color::Green;
        case 'B':
            return (std::uint8_t)Color::Blue;
        case 'W':
            return (std::uint8_t)Color::White;
        case 'O':
            return (std::uint8_t)Color::Orange;
        case 'Y':
            return (std::uint8_t)Color::Yellow;
        case ' ':
        case 'E':
            return (std::uint8_t)Color::Empty;
        case '?':
            return (std::uint8_t)Color::Unknown;
        default:
            return (std::uint8_t)Color::Illegal;
        }
    }

    static char toChar(std::size_t color) {
        switch (color) {
            case Color::Red:
                return 'R';
            case Color::Green:
                return 'G';
            case Color::Blue:
                return 'B';
            case Color::White:
                return 'W';
            case Color::Orange:
                return 'O';
            case Color::Yellow:
                return 'Y';
            case Color::Empty:
                return 'E';
            case Color::Unknown:
                return '?';
            default:
                return '*';
        }
    }

    static const char * toString(std::size_t color) {
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
