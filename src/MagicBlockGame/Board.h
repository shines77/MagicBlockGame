#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstring>      // For std::memset()
#include <type_traits>  // For std::conditional<bool, T1, T2>
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

inline
bool operator >= (const uint128_t & lhs, const uint128_t & rhs) noexcept  {
    return (lhs.compare(rhs) == -1);
}

inline
bool operator <= (const uint128_t & lhs, const uint128_t & rhs) noexcept  {
    return (lhs.compare(rhs) == 1);
}

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
        for (size_t n = 0; n < kUnits; n++) {
            this->uints[n] = other.uints[n];
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
        for (size_t n = 0; n < kUnits; n++) {
            this->uints[n] = other.uints[n];
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
