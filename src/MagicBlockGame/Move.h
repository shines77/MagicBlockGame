#pragma once

#include <stdint.h>
#include <stddef.h>

namespace PuzzleGame {

#pragma pack(push, 1)

struct Position8 {
    int8_t x;
    int8_t y;

    Position8() : x(0), y(0) {}
    Position8(int _x, int _y) : x(_x), y(_y) {}
    Position8(const Position8 & src) {
        this->x = src.x;
        this->y = src.y;
    }
};

struct Position {
    int16_t value;

    Position() : value(0) {}
    Position(int16_t _value) : value(_value) {}
    Position(int32_t _value) : value(static_cast<int16_t>(_value)) {}
    Position(int64_t _value) : value(static_cast<int16_t>(_value)) {}
    Position(const Position & src) {
        this->value = src.value;
    }

    Position & operator = (const Position & rhs) {
        this->value = rhs.value;
        return *this;
    }

    Position & operator = (int64_t rhs) {
        this->value = static_cast<int16_t>(rhs);
        return *this;
    }

    Position & operator = (int32_t rhs) {
        this->value = static_cast<int16_t>(rhs);
        return *this;
    }

    Position & operator = (int16_t rhs) {
        this->value = rhs;
        return *this;
    }
};

struct Move {
    Position    pos;
    uint8_t     dir;
    uint8_t     reserve;
};

struct MoveInfo {
    Position    from_pos;
    Position    move_to_pos;
    uint8_t     dir;
    uint8_t     reserve;
};

struct Offset {
    int x;
    int y;
};
    
static const Offset Dir_Offset[] = {
    {  0,  1 },
    { -1,  0 },
    {  0, -1 },
    {  1,  0 }
};

struct Direction {
    enum {
        First,
        Down = First,
        Left,
        Up,
        Right,
        Last
    };

    template <size_t BoardX, size_t BoardY>
    static uint8_t getDir(Position from, Position to) {
        int from_x = (int)from.value % (int)BoardY;
        int from_y = (int)from.value / (int)BoardY;
        int to_x = (int)to.value % (int)BoardY;
        int to_y = (int)to.value / (int)BoardY;

        int offset_x = to_x - from_x;
        int offset_y = to_y - from_y;

        for (size_t dir = Direction::First; dir < Direction::Last; dir++) {
            if ((offset_x == Dir_Offset[dir].x) &&
                (offset_y == Dir_Offset[dir].y)) {
                return (uint8_t)dir;
            }
        }

        return uint8_t(-1);
    }

    static const char * toString(size_t dir) {
        switch (dir) {
            case Direction::Down:
                return "Down";
            case Direction::Left:
                return "Left";
            case Direction::Up:
                return "Up";
            case Direction::Right:
                return "Right";
            default:
                return "Unknown";
        }
    }
};

#pragma pack(pop)

} // namespace PuzzleGame
