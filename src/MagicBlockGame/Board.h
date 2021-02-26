#pragma once

#include <stdint.h>
#include <stddef.h>

#include <algorithm>    // For std::fill_n()

#include "Color.h"

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

    bool is_equal(const Board & other) const {
        for (size_t cell = 0; cell < (BoardX * BoardY); cell++) {
            if (this->cells[cell] != other.cells[cell])
                return false;
        }
        return true;
    }

    bool is_not_equal(const Board & other) const {
        for (size_t cell = 0; cell < (BoardX * BoardY); cell++) {
            if (this->cells[cell] != other.cells[cell])
                return true;
        }
        return false;
    }

    size_t value() const {
        size_t bit_value = 0;
        for (size_t cell = 0; cell < (BoardX * BoardY); cell++) {
            bit_value <<= 3;
            bit_value |= this->cells[cell];
        }
        return bit_value;
    }
};

template <size_t BoardX, size_t BoardY>
inline
bool operator == (const Board<BoardX, BoardY> & lhs, const Board<BoardX, BoardY> & rhs) {
    return lhs.is_equal(rhs);
}

template <size_t BoardX, size_t BoardY>
inline
bool operator != (const Board<BoardX, BoardY> & lhs, const Board<BoardX, BoardY> & rhs) {
    return lhs.is_not_equal(rhs);
}
