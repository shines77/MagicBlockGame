#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstdint>
#include <cstddef>
#include <cstring>      // For std::memset()
#include <type_traits>  // For std::conditional<bool, T1, T2>
#include <algorithm>    // For std::fill_n()

#include "Color.h"
#include "Value128.h"

namespace MagicBlock {

template <std::size_t BoardX, std::size_t BoardY>
union Board
{
    typedef std::size_t size_type;

    static const size_type X = BoardX;
    static const size_type Y = BoardY;
    static const size_type BoardSize = BoardX * BoardY;
    static const size_type Bytes = BoardX * BoardY;

    typedef typename std::conditional<
                (Bytes <= sizeof(std::uint32_t)), std::uint32_t, size_type
            >::type  unit_type;
    typedef Board<BoardX, BoardY> this_type;

    static const size_type kUnitBytes = sizeof(unit_type);

    static const size_type kUnits = (Bytes + kUnitBytes - 1) / kUnitBytes;
    static const size_type kBytes = kUnits * kUnitBytes;

    std::uint8_t    cells[BoardX * BoardY];
    unit_type       uints[kUnits];

    Board() {
        this->clear();
    }
    Board(const Board & src) noexcept {
        this->internal_copy(src);
    }
    Board(Board && src) noexcept {
        this->internal_swap(src);
    }

    ~Board() {}

    Board & operator = (const Board & rhs) noexcept {
        this->copy(rhs);
        return *this;
    }

    Board & operator = (Board && rhs) noexcept {
        this->swap(rhs);
        return *this;
    }

    void clear() noexcept {
#if 0
        std::fill_n(this->cells, sizeof(cells), Color::Empty);
#else
        for (size_type n = 0; n < kUnits; n++) {
            this->uints[n] = 0;
        }
#endif
    }

    void internal_copy(const Board & other) noexcept {
        for (size_type n = 0; n < kUnits; n++) {
            this->uints[n] = other.uints[n];
        }
    }

    void copy(const Board & other) noexcept {
        if (&other != this) {
            this->internal_copy(other);
        }
    }

    void copy(uint8_t cells[BoardX * BoardY]) noexcept {
        size_type cell;
        for (cell = 0; cell < BoardSize; cell++) {
            this->cells[cell] = cells[cell];
        }
        for (; cell < kBytes; cell++) {
            this->cells[cell] = 0;
        }
    }

    void internal_swap(Board & other) noexcept {
        for (size_type n = 0; n < kUnits; n++) {
            unit_type temp = this->uints[n];
            this->uints[n] = other.uints[n];
            other.uints[n] = temp;
        }
    }

    void swap(Board & other) noexcept {
        if (&other != this) {
            this->internal_swap(other);
        }
    }

    bool is_equal(const Board & other) const noexcept {
#if 1
        for (size_type n = 0; n < kUnits; n++) {
            if (this->uints[n] != other.uints[n])
                return false;
        }
#else
        for (size_type cell = 0; cell < BoardSize; cell++) {
            if (this->cells[cell] != other.cells[cell])
                return false;
        }
#endif
        return true;
    }

    size_type value() const noexcept {
        size_type value64 = 0;
        for (size_type cell = 0; cell < BoardSize; cell++) {
            value64 <<= 3;
            value64 |= (this->cells[cell] & 0x07U);
        }
        return value64;
    }

    Value128 value128() const noexcept {
        std::uint64_t low = 0, high = 0;
        if (BoardSize <= 21) {
            for (std::ptrdiff_t cell = BoardSize - 1; cell >= 0; cell--) {
                low <<= 3;
                low |= std::uint64_t(this->cells[cell] & 0x07U);
            }
        }
        else {
            // Low: bit 0 ~ 62, 21 * 3 = 63 bits
            for (std::ptrdiff_t cell = 20; cell >= 0; cell--) {
                low <<= 3;
                low |= std::uint64_t(this->cells[cell] & 0x07U);
            }
            // Low: bit 63
            low |= std::uint64_t(this->cells[21] & 0x01U) << 63;

            // High: bit 2 ~ 63
            for (std::ptrdiff_t cell = BoardSize - 1; cell >= 21; cell--) {
                high <<= 3;
                high |= std::uint64_t(this->cells[cell] & 0x07U);
            }

            // High: bit 0 ~ 1
            high >>= 1;
        }
        return Value128(low, high);
    }

    // clockwise rotate 90 degrees
    void rotate_90_cw(Board<BoardX, BoardY> & dest) {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                size_type src_pos = y * BoardY + x;
                size_type dest_x = (BoardY - 1) - y;
                size_type dest_y = x;
                size_type dest_pos = dest_y * BoardY + dest_x;
                dest.cells[dest_pos] = this->cells[src_pos];
            }
        }
    }

    // clockwise rotate 180 degrees
    void rotate_180_cw(Board<BoardX, BoardY> & dest) {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                size_type src_pos = y * BoardY + x;
                size_type dest_x = (BoardX - 1) - x;
                size_type dest_y = (BoardY - 1) - y;
                size_type dest_pos = dest_y * BoardY + dest_x;
                dest.cells[dest_pos] = this->cells[src_pos];
            }
        }
    }

    // clockwise rotate 270 degrees
    void rotate_270_cw(Board<BoardX, BoardY> & dest) {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                size_type src_pos = y * BoardY + x;
                size_type dest_x = y;
                size_type dest_y = (BoardX - 1) - x;
                size_type dest_pos = dest_y * BoardY + dest_x;
                dest.cells[dest_pos] = this->cells[src_pos];
            }
        }
    }
};

template <std::size_t BoardX, std::size_t BoardY>
inline
bool operator == (const Board<BoardX, BoardY> & lhs, const Board<BoardX, BoardY> & rhs) noexcept {
    return lhs.is_equal(rhs);
}

template <std::size_t BoardX, std::size_t BoardY>
inline
bool operator != (const Board<BoardX, BoardY> & lhs, const Board<BoardX, BoardY> & rhs) noexcept {
    return !(lhs.is_equal(rhs));
}

template <std::size_t BoardX, std::size_t BoardY>
inline
void swap(Board<BoardX, BoardY> & lhs, Board<BoardX, BoardY> & rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace MagicBlock
