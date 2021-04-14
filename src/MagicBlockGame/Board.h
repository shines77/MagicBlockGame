#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstring>      // For std::memset()
#include <type_traits>  // For std::conditional<bool, T1, T2>
#include <algorithm>    // For std::fill_n()

#include "Color.h"
#include "UInt128.h"

namespace PuzzleGame {

template <size_t BoardX, size_t BoardY>
union Board
{
    static const size_t Bytes = BoardX * BoardY;

    typedef typename std::conditional<
                (Bytes <= sizeof(uint32_t)), uint32_t, size_t
            >::type  unit_type;
    typedef Board<BoardX, BoardY> this_type;

    static const size_t kUnitBytes = sizeof(unit_type);

    static const size_t kUnits = (Bytes + kUnitBytes - 1) / kUnitBytes;
    static const size_t kBytes = kUnits * kUnitBytes;

    uint8_t     cells[BoardX * BoardY];
    unit_type   uints[kUnits];

    Board() {
        this->clear();
    }
    Board(const Board & other) {
        this->copy(other);
    }

    ~Board() {}

    void clear() {
#if 0
        std::fill_n(this->cells, sizeof(cells), Color::Empty);
#else
        for (size_t n = 0; n < kUnits; n++) {
            this->uints[n] = 0;
        }
#endif
    }

    void copy(const Board & other) {
        if (this != &other) {
            for (size_t n = 0; n < kUnits; n++) {
                this->uints[n] = other.uints[n];
            }
        }
    }

    void copy(uint8_t cells[BoardX * BoardY]) {
        size_t cell;
        for (cell = 0; cell < (BoardX * BoardY); cell++) {
            this->cells[cell] = cells[cell];
        }
        for (; cell < kBytes; cell++) {
            this->cells[cell] = 0;
        }
    }

    Board & operator = (const Board & other) {
        if (this != &other) {
            for (size_t n = 0; n < kUnits; n++) {
                this->uints[n] = other.uints[n];
            }
        }
        return *this;
    }

    bool is_equal(const Board & other) const noexcept {
#if 1
        for (size_t n = 0; n < kUnits; n++) {
            if (this->uints[n] != other.uints[n])
                return false;
        }
#else
        for (size_t cell = 0; cell < (BoardX * BoardY); cell++) {
            if (this->cells[cell] != other.cells[cell])
                return false;
        }
#endif
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
            for (ptrdiff_t cell = BoardX * BoardY - 1; cell >= 0; cell--) {
                low_value <<= 3;
                low_value |= uint64_t(this->cells[cell] & 0x07ULL);
            }
        }
        else {
            // Low: bit 0 ~ 62, 21 * 3 = 63 bits
            for (ptrdiff_t cell = 20; cell >= 0; cell--) {
                low_value <<= 3;
                low_value |= uint64_t(this->cells[cell] & 0x07ULL);
            }
            // Low: bit 63
            low_value |= uint64_t(this->cells[21] & 0x01ULL) << 63;

            // High: bit 2 ~ 63
            for (ptrdiff_t cell = BoardX * BoardY - 1; cell >= 21; cell--) {
                high_value <<= 3;
                high_value |= uint64_t(this->cells[cell] & 0x07ULL);
            }

            // High: bit 0 ~ 1
            high_value >>= 1;
        }
        return uint128_t(low_value, high_value);
    }

    // clockwise rotate 90 degrees
    void rotate_90_cw(Board<BoardX, BoardY> & dest) {
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                size_t src_pos = y * BoardY + x;
                size_t dest_x = (BoardY - 1) - y;
                size_t dest_y = x;
                size_t dest_pos = dest_y * BoardY + dest_x;
                dest.cells[dest_pos] = this->cells[src_pos];
            }
        }
    }

    // clockwise rotate 180 degrees
    void rotate_180_cw(Board<BoardX, BoardY> & dest) {
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                size_t src_pos = y * BoardY + x;
                size_t dest_x = (BoardX - 1) - x;
                size_t dest_y = (BoardY - 1) - y;
                size_t dest_pos = dest_y * BoardY + dest_x;
                dest.cells[dest_pos] = this->cells[src_pos];
            }
        }
    }

    // clockwise rotate 270 degrees
    void rotate_270_cw(Board<BoardX, BoardY> & dest) {
        for (size_t y = 0; y < BoardY; y++) {
            for (size_t x = 0; x < BoardX; x++) {
                size_t src_pos = y * BoardY + x;
                size_t dest_x = y;
                size_t dest_y = (BoardX - 1) - x;
                size_t dest_pos = dest_y * BoardY + dest_x;
                dest.cells[dest_pos] = this->cells[src_pos];
            }
        }
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
