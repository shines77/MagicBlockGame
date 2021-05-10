#pragma once

#include <stdint.h>
#include <stddef.h>

#include <type_traits>

namespace MagicBlock {
namespace AI {

union size_u {
    typedef typename std::conditional<
                (sizeof(size_t) == 4), uint16_t, uint32_t
            >::type  half_type;
    struct {
        half_type low, high;
    };

    size_t value;

    size_u(size_t _value) : value(_value) {}
    size_u(size_t _low, size_t _high) : low(half_type(_low)), high(half_type(_high)) {}
    ~size_u() {}

    size_u & operator = (size_t rhs) {
        this->value = rhs;
        return *this;
    }
};

struct Value128 {
    uint64_t low;
    uint64_t high;

    Value128() noexcept : low(0), high(0) {}
    Value128(uint64_t _low, uint64_t _high) noexcept : low(_low), high(_high) {}
    Value128(const Value128 & other) noexcept : low(0), high(0) {
        this->low  = other.low;
        this->high = other.high;
    }
    ~Value128() {}

    bool is_equal(const Value128 & other) const noexcept {
        return ((this->low == other.low) && (this->high == other.high));
    }

    int compare(const Value128 & other) const noexcept {
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
bool operator == (const Value128 & lhs, const Value128 & rhs) noexcept  {
    return lhs.is_equal(rhs);
}

inline
bool operator != (const Value128 & lhs, Value128 & rhs) noexcept {
    return !(lhs.is_equal(rhs));
}

inline
bool operator > (const Value128 & lhs, const Value128 & rhs) noexcept  {
    return (lhs.compare(rhs) == 1);
}

inline
bool operator < (const Value128 & lhs, const Value128 & rhs) noexcept  {
    return (lhs.compare(rhs) == -1);
}

inline
bool operator >= (const Value128 & lhs, const Value128 & rhs) noexcept  {
    return (lhs.compare(rhs) == -1);
}

inline
bool operator <= (const Value128 & lhs, const Value128 & rhs) noexcept  {
    return (lhs.compare(rhs) == 1);
}

} // namespace AI
} // namespace MagicBlock
