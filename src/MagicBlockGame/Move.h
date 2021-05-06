#pragma once

#include <stdint.h>
#include <stddef.h>

namespace MagicBlock {

#pragma pack(push, 1)

struct Position8 {
    uint8_t x;
    uint8_t y;

    Position8() : x(0), y(0) {}
    Position8(int _x, int _y) : x(static_cast<uint8_t>(_x)), y(static_cast<uint8_t>(_y)) {}

    Position8(const Position8 & src) {
        this->x = src.x;
        this->y = src.y;
    }

    ~Position8() {}
};

struct Position {
    uint8_t value;

    Position() : value(0) {}
    Position(int8_t _value) : value(static_cast<uint8_t>(_value)) {}
    Position(uint8_t _value) : value(_value) {}
    Position(int16_t _value) : value(static_cast<uint8_t>(_value)) {}
    Position(uint16_t _value) : value(static_cast<uint8_t>(_value)) {}
    Position(int32_t _value) : value(static_cast<uint8_t>(_value)) {}
    Position(uint32_t _value) : value(static_cast<uint8_t>(_value)) {}
    Position(int64_t _value) : value(static_cast<uint8_t>(_value)) {}
    Position(uint64_t _value) : value(static_cast<uint8_t>(_value)) {}

    Position(const Position & src) {
        this->value = src.value;
    }
    Position(Position && src) {
        std::swap(this->value, src.value);
    }

    ~Position() {}

    Position & operator = (const Position & rhs) {
        this->value = rhs.value;
        return *this;
    }

    Position & operator = (Position && rhs) {
        this->value = rhs.value;
        return *this;
    }

    Position & operator = (int8_t rhs) {
        this->value = static_cast<uint8_t>(rhs);
        return *this;
    }

    Position & operator = (uint8_t rhs) {
        this->value = rhs;
        return *this;
    }

    Position & operator = (int16_t rhs) {
        this->value = static_cast<uint8_t>(rhs);
        return *this;
    }

    Position & operator = (uint16_t rhs) {
        this->value = static_cast<uint8_t>(rhs);
        return *this;
    }

    Position & operator = (int32_t rhs) {
        this->value = static_cast<uint8_t>(rhs);
        return *this;
    }

    Position & operator = (uint32_t rhs) {
        this->value = static_cast<uint8_t>(rhs);
        return *this;
    }

    Position & operator = (int64_t rhs) {
        this->value = static_cast<uint8_t>(rhs);
        return *this;
    }

    Position & operator = (uint64_t rhs) {
        this->value = static_cast<uint8_t>(rhs);
        return *this;
    }

    operator int8_t () const {
        return static_cast<int8_t>(this->value);
    }

    operator uint8_t () const {
        return this->value;
    }

    operator int16_t () const {
        return static_cast<int16_t>(this->value);
    }

    operator uint16_t () const {
        return static_cast<uint16_t>(this->value);
    }

    operator int32_t () const {
        return static_cast<int32_t>(this->value);
    }

    operator uint32_t () const {
        return this->value;
    }

    operator int64_t () const {
        return static_cast<int64_t>(this->value);
    }

    operator uint64_t () const {
        return static_cast<uint64_t>(this->value);
    }

    void swap(Position & other) {
        std::swap(this->value, other.value);
    }

    static const char posToChr(size_t pos) {
        if (pos != uint8_t(-1))
            return (char)('A' + (uint8_t)(pos % 256));
        else
            return '?';
    }
};

struct Move {
    Position    pos;
    uint8_t     dir;

    Move() : pos(0), dir(0) {}
    Move(const Move & src) {
        this->pos = src.pos;
        this->dir = src.dir;
    }
    Move(Move && src) {
        this->pos.swap(src.pos);
        std::swap(this->dir, src.dir);
    }
    ~Move() {}

    Move & operator = (const Move & rhs) {
        this->pos = rhs.pos;
        this->dir = rhs.dir;
        return *this;
    }
};

struct MoveInfo {
    Position    from_pos;
    Position    move_pos;
    uint8_t     color;
    uint8_t     dir;

    MoveInfo() : from_pos(0), move_pos(0), color(Color::Empty), dir(0) {}
    MoveInfo(const MoveInfo & src) {
        this->from_pos = src.from_pos;
        this->move_pos = src.move_pos;
        this->color = src.color;
        this->dir = src.dir;
    }
    MoveInfo(MoveInfo && src) {
        this->from_pos.swap(src.from_pos);
        this->move_pos.swap(src.move_pos);
        std::swap(this->color, src.color);
        std::swap(this->dir, src.dir);
    }
    ~MoveInfo() {}

    MoveInfo & operator = (const MoveInfo & rhs) {
        this->from_pos = rhs.from_pos;
        this->move_pos = rhs.move_pos;
        this->color = rhs.color;
        this->dir = rhs.dir;
        return *this;
    }
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
        Unknown,
        Last = Unknown
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

    // Get the opposite direction
    static uint8_t getOppDir(uint8_t dir) {
        switch (dir) {
            case Direction::Down:
                return (uint8_t)Direction::Up;
            case Direction::Left:
                return (uint8_t)Direction::Right;
            case Direction::Up:
                return (uint8_t)Direction::Down;
            case Direction::Right:
                return (uint8_t)Direction::Left;
            default:
                return (uint8_t)Direction::Unknown;
        }
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

} // namespace MagicBlock
