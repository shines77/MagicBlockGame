#pragma once

#include <stdint.h>
#include <stddef.h>

#include <type_traits>

namespace PuzzleGame {

static const size_t MaxRotateType = 4;
static const size_t MaxPhrase1Type = 4;

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

    size_u & operator = (size_t rhs) {
        this->value = rhs;
        return *this;
    }
};

} // namespace PuzzleGame
