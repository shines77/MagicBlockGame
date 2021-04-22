
#ifndef JSTD_SUPPORT_RT_POWEROF2_H
#define JSTD_SUPPORT_RT_POWEROF2_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>      // For std::size_t
#include <cassert>
#include <type_traits>
#include <limits>       // For std::numeric_limits<T>::max()

//////////////////////////////////////////////////////////////////////////
//
// Bit Twiddling Hacks
//
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
//
//////////////////////////////////////////////////////////////////////////

namespace jstd {
namespace run_time {

} // namespace run_time
} // namespace jstd

#endif // JSTD_SUPPORT_RT_POWEROF2_H