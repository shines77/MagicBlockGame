#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstring>      // For std::memset()
#include <algorithm>    // For std::fill_n()

#include "Color.h"

namespace PuzzleGame {

struct uint128_t {
    uint64_t low;
    uint64_t high;

    uint128_t() noexcept : low(0), high(0) {}
    uint128_t(uint64_t _low, uint64_t _high) noexcept : low(_low), high(_high) {}
    uint128_t(const uint128_t & other) noexcept : low(0), high(0) {
        this->low  = other.low;
        this->high = other.high;
    }

    bool is_equal(const uint128_t & other) const noexcept {
        return ((this->low == other.low) && (this->high == other.high));
    }

    int compare(const uint128_t & other) const noexcept {
        if (this->high > other.high) {
            return 1;
        }
        else if (this->high == other.high) {
            if (this->low > other.low)
                return 1;
            else if (this->low < other.low)
                return -1;
            else
                return 0;
        }
        else {
            return -1;
        }
    }
};

inline
bool operator == (const uint128_t & lhs, const uint128_t & rhs) noexcept  {
    return lhs.is_equal(rhs);
}

inline
bool operator != (const uint128_t & lhs, uint128_t & rhs) noexcept {
    return !(lhs.is_equal(rhs));
}

inline
bool operator > (const uint128_t & lhs, const uint128_t & rhs) noexcept  {
    return (lhs.compare(rhs) == 1);
}

inline
bool operator < (const uint128_t & lhs, const uint128_t & rhs) noexcept  {
    return (lhs.compare(rhs) == -1);
}

template <size_t BoardX, size_t BoardY>
struct Board
{
    uint8_t cells[BoardX * BoardY];

    Board() {
        this->clear();
    }
    Board(const Board & other) {
        this->copy(other);
    }

    ~Board() {}

    void clear() {
        std::fill_n(this->cells, sizeof(cells), Color::Empty);
    }

    void copy(const Board & other) {
        for (size_t cell = 0; cell < (BoardX * BoardY); cell++) {
            this->cells[cell] = other.cells[cell];
        }
    }

    void copy(uint8_t cells[BoardX * BoardY]) {
        for (size_t cell = 0; cell < (BoardX * BoardY); cell++) {
            this->cells[cell] = cells[cell];
        }
    }

    Board & operator = (const Board & other) {
        for (size_t cell = 0; cell < (BoardX * BoardY); cell++) {
            this->cells[cell] = other.cells[cell];
        }
        return *this;
    }

    bool is_equal(const Board & other) const noexcept {
        for (size_t cell = 0; cell < (BoardX * BoardY); cell++) {
            if (this->cells[cell] != other.cells[cell])
                return false;
        }
        return true;
    }

    size_t value() const noexcept {
        size_t value64 = 0;
        for (size_t cell = 0; cell < (BoardX * BoardY); cell++) {
            value64 <<= 3;
            value64 |= (this->cells[cell] & 0x07UL);
        }
        return value64;
    }

    uint128_t value128() const noexcept {
        uint64_t low_value = 0, high_value = 0;
        if (BoardX * BoardY <= 21) {
            for (size_t cell = 0; cell < (BoardX * BoardY); cell++) {
                low_value <<= 3;
                low_value |= (this->cells[cell] & 0x07UL);
            }
        }
        else {
            // Low: 21 * 3 = 63 bits
            for (size_t cell = 0; cell < 21; cell++) {
                low_value <<= 3;
                low_value |= (this->cells[cell] & 0x07UL);
            }
        }
        return uint128_t(low_value, high_value);
    }
};

template <size_t BoardX, size_t BoardY>
inline
bool operator == (const Board<BoardX, BoardY> & lhs, const Board<BoardX, BoardY> & rhs) noexcept  {
    return lhs.is_equal(rhs);
}

template <size_t BoardX, size_t BoardY>
inline
bool operator != (const Board<BoardX, BoardY> & lhs, const Board<BoardX, BoardY> & rhs) noexcept {
    return !(lhs.is_equal(rhs));
}

} // namespace PuzzleGame
