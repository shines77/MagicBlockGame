#pragma once

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

    static char valToColor(char value) {
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
