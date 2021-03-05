#pragma once

#include <stdint.h>
#include <stddef.h>

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

} // namespace PuzzleGame
