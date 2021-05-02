
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

#include "support/CT_PowerOf2.h"    // For jstd::integral_traits<T>

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__)
#define __IS_X86_64     1
#endif // __amd64__

#if defined(_MSC_VER) || (defined(WIN32) && (defined(__INTEL_COMPILER) || defined(__ICL)))
#include <intrin.h>     // For _BitScanReverse(), _BitScanReverse64()
                        // For _BitScanForward(), _BitScanForward64()
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)
#if __IS_X86_64
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(_BitScanForward64)
#endif // __amd64__
#endif // _MSC_VER

//
// See: http://www.cnblogs.com/zyl910/archive/2012/08/27/intrin_table_gcc.html
//
//#include <xmmintrin.h>    // For MMX, SSE instructions
//#include <emmintrin.h>    // For SSE2 instructions, __SSE2__ | -msse2
//#include <avxintrin.h>    // __AVX__  | -mavx     AVX:  Advanced Vector Extensions
//#include <avx2intrin.h>   // __AVX2__ | -mavx2    AVX2: Advanced Vector Extensions 2
//

/* __has_builtin() available in clang */
#ifdef __has_builtin
#  if __has_builtin(__builtin_clz)
#    define __has_builtin_clz
#  endif
#  if __has_builtin(__builtin_clzll)
#    define __has_builtin_clzll
#  endif
/* __builtin_clz available beginning with GCC 3.4 */
#elif (__GNUC__ * 100 + __GNUC_MINOR__) >= 304
#  define __has_builtin_clz
#  define __has_builtin_clzll
#endif // __has_builtin

/* __has_builtin() available in clang */
#ifdef __has_builtin
#  if __has_builtin(__builtin_ctz)
#    define __has_builtin_ctz
#  endif
#  if __has_builtin(__builtin_ctzll)
#    define __has_builtin_ctzll
#  endif
/* __builtin_ctz available beginning with GCC 3.4 */
#elif (__GNUC__ * 100 + __GNUC_MINOR__) >= 304
#  define __has_builtin_ctz
#  define __has_builtin_ctzll
#endif // __has_builtin

#define JSTD_SUPPORT_X86_BITSCAN_INSTRUCTION    1

//////////////////////////////////////////////////////////////////////////
//
// Bit Twiddling Hacks
//
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
//
//////////////////////////////////////////////////////////////////////////

namespace jstd {
namespace run_time {

//////////////////////////////////////////////////////////////
// popcount()

//
// popcount() algorithm
//
// See: http://www.cnblogs.com/Martinium/articles/popcount.html
// See: https://en.wikipedia.org/wiki/Hamming_weight
// See: https://stackoverflow.com/questions/757059/position-of-least-significant-bit-that-is-set
//

static inline
unsigned int internal_popcnt(uint32_t x)
{ 
    x -=  ((x >> 1U) & 0x55555555U);
    x  = (((x >> 2U) & 0x33333333U) + (x & 0x33333333U));
    x  = (((x >> 4U) + x) & 0x0F0F0F0FU);
    x +=   (x >> 8U);
    x +=   (x >> 16U);
    x  = x & 0x0000003FU;
    assert(x >= 0 && x <= 32);
    return x;
}

static inline
unsigned int internal_popcnt64(uint64_t x)
{
#if 1
    x -=  ((x >> 1U) & 0x55555555U);
    x  = (((x >> 2U) & 0x33333333U) + (x & 0x33333333U));
    x  = (((x >> 4U) + x) & 0x0F0F0F0FU);
    x +=   (x >> 8U);
    x +=   (x >> 16U);
    x +=   (x >> 32U);
    x  = x & 0x0000007FU;
    assert(x >= 0 && x <= 64);
    return (unsigned int)x;
#elif 0
    x = (x & 0x5555555555555555ULL) + ((x >>  1U) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >>  2U) & 0x3333333333333333ULL);
    x = (x & 0x0F0F0F0F0F0F0F0FULL) + ((x >>  4U) & 0x0F0F0F0F0F0F0F0FULL);
    x = (x & 0x00FF00FF00FF00FFULL) + ((x >>  8U) & 0x00FF00FF00FF00FFULL);
    x = (x & 0x0000FFFF0000FFFFULL) + ((x >> 16U) & 0x0000FFFF0000FFFFULL);
    x = (x & 0x00000000FFFFFFFFULL) + ((x >> 32U) & 0x00000000FFFFFFFFULL);
    assert(x >= 0 && x <= 64);
    return (unsigned int)x;
#else
    unsigned int high, low;
    unsigned int n1, n2;
    high = (unsigned int) (x & 0x00000000FFFFFFFFULL);
    low  = (unsigned int)((x & 0xFFFFFFFF00000000ULL) >> 32U);
    n1 = internal_popcnt(high);
    n2 = internal_popcnt(low);
    return (n1 + n2);
#endif
}

static inline
int internal_ctz(uint32_t x)
{
    return (int)internal_popcnt((x & -(int)x) - 1);
}

static inline
int internal_ctzll(uint64_t x)
{
    return (int)internal_popcnt64((x & -(int64_t)x) - 1);
}

static inline
int internal_clz(uint32_t x)
{
    // Hacker's Delight, 2nd ed. Fig 5-16, p. 102
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return (int)internal_popcnt(~x);
}

static inline
int internal_clzll(uint64_t x)
{
    // Hacker's Delight, 2nd ed. Fig 5-16, p. 102
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    x |= (x >> 32);
    return (int)internal_popcnt64(~x);
}

static inline
unsigned int internal_BitScanReverse(uint32_t x)
{
    // Hacker's Delight, 2nd ed. Fig 5-16, p. 102
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return run_time::internal_popcnt(x);
}

static inline
unsigned int internal_BitScanReverse(uint64_t x)
{
    // Hacker's Delight, 2nd ed. Fig 5-16, p. 102
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    x |= (x >> 32);
    return run_time::internal_popcnt64(x);
}

//////////////////////////////////////////////////////////////
// BitScanForward

static inline
bool BitScanForward(std::uint32_t mask, unsigned int * index)
{
    assert(index != nullptr);
#if defined(_MSC_VER)
    unsigned char non_zero = ::_BitScanForward((unsigned long *)index, mask);
    return (non_zero != 0);
#elif defined(__has_builtin_ctz)
    int trailing_zeros = __builtin_ctz((unsigned int)mask);
    *index = (unsigned int)trailing_zeros;
    return (mask != 0);
#else
    int trailing_zeros = run_time::internal_ctz((unsigned int)mask);
    *index = (unsigned int)trailing_zeros;
    return (mask != 0);
#endif
}

static inline
int BitScanForward_nonzero(std::uint32_t mask)
{
    assert(mask != 0);
#if defined(_MSC_VER)
    unsigned long index;
    unsigned char non_zero = ::_BitScanForward(&index, mask);
    (void)non_zero;
    return (int)index;
#elif defined(__has_builtin_ctz)
    int trailing_zeros = __builtin_ctz((unsigned int)mask);
    return trailing_zeros;
#else
    int trailing_zeros = run_time::internal_ctz((unsigned int)mask);
    return trailing_zeros;
#endif
}

#if __IS_X86_64

static inline
bool BitScanForward(std::uint64_t mask, unsigned int * index)
{
    assert(index != nullptr);
#if defined(_MSC_VER)
    unsigned char non_zero = ::_BitScanForward64((unsigned long *)index, mask);
    return (non_zero != 0);
#elif defined(__has_builtin_ctzll)
    int trailing_zeros = __builtin_ctzll((unsigned long long)mask);
    *index = (unsigned int)trailing_zeros;
    return (mask != 0);
#else
    int trailing_zeros = run_time::internal_ctzll(mask);
    *index = (unsigned int)trailing_zeros;
    return (mask != 0);
#endif
}

static inline
int BitScanForward_nonzero(std::uint64_t mask)
{
    assert(mask != 0);
#if defined(_MSC_VER)
    unsigned long index;
    unsigned char non_zero = ::_BitScanForward64(&index, mask);
    (void)non_zero;
    return (int)index;
#elif defined(__has_builtin_ctzll)
    int trailing_zeros = __builtin_ctzll((unsigned int)mask);
    return trailing_zeros;
#else
    int trailing_zeros = run_time::internal_ctzll(mask);
    return trailing_zeros;
#endif
}

#endif // __amd64__

//////////////////////////////////////////////////////////////
// BitScanReverse

static inline
bool BitScanReverse(std::uint32_t mask, unsigned int * index)
{
    assert(index != nullptr);
#if defined(_MSC_VER)
    unsigned char non_zero = ::_BitScanReverse((unsigned long *)index, mask);
    return (non_zero != 0);
#elif defined(__has_builtin_clz)
    int leading_zeros = __builtin_clz((unsigned int)mask);
    *index = (unsigned int)(31UL ^ (unsigned int)leading_zeros);
    return (mask != 0);
#else
    *index = (unsigned long)internal_BitScanReverse(mask);
    return (mask != 0);
#endif
}

static inline
int BitScanReverse_nonzero(std::uint32_t mask)
{
    assert(mask != 0);
#if defined(_MSC_VER)
    unsigned long index;
    unsigned char non_zero = ::_BitScanReverse(&index, mask);
    (void)non_zero;
    return (int)index;
#elif defined(__has_builtin_clz)
    int leading_zeros = __builtin_clz((unsigned int)mask);
    int index = (int)(31UL ^ (unsigned int)leading_zeros);
    return index;
#else
    return (int)internal_BitScanReverse(mask);
#endif
}

#if __IS_X86_64

static inline
bool BitScanReverse(std::uint64_t mask, unsigned int * index)
{
    assert(index != nullptr);
#if defined(_MSC_VER)
    unsigned char non_zero = ::_BitScanReverse64((unsigned long *)index, mask);
    return (non_zero != 0);
#elif defined(__has_builtin_clzll)
    int leading_zeros = __builtin_clzll((unsigned long long)mask);
    *index = (unsigned int)(63UL ^ (unsigned int)leading_zeros);
    return (mask != 0);
#else
    *index = (unsigned long)internal_BitScanReverse(mask);
    return (mask != 0);
#endif
}

static inline
int BitScanReverse_nonzero(std::uint64_t mask)
{
    assert(mask != 0);
#if defined(_MSC_VER)
    unsigned long index;
    unsigned char non_zero = ::_BitScanReverse64(&index, mask);
    (void)non_zero;
    return (int)index;
#elif defined(__has_builtin_clzll)
    int leading_zeros = __builtin_clzll((unsigned long long)mask);
    int index = (int)(63UL ^ (unsigned int)leading_zeros);
    return index;
#else
    return (int)internal_BitScanReverse(mask);
#endif
}

#endif // __amd64__

//////////////////////////////////////////////////////////////
// CountTrailingZeroBits

static inline
bool CountTrailingZeroBits(std::uint32_t mask, unsigned int * index)
{
    return run_time::BitScanForward(mask, index);
}

static inline
int CountTrailingZeroBits_nonzero(std::uint32_t mask)
{
    return run_time::BitScanForward_nonzero(mask);
}

#if __IS_X86_64

static inline
bool CountTrailingZeroBits(std::uint64_t mask, unsigned int * index)
{
    return run_time::BitScanForward(mask, index);
}

static inline
int CountTrailingZeroBits_nonzero(std::uint64_t mask)
{
    return run_time::BitScanForward_nonzero(mask);
}

#endif // __amd64__

//////////////////////////////////////////////////////////////
// CountLeadingZeroBits

static inline
bool CountLeadingZeroBits(std::uint32_t mask, unsigned int * index)
{
    assert(index != nullptr);
#if defined(_MSC_VER)
    unsigned long result;
    unsigned char non_zero = ::_BitScanReverse(&result, mask);
    // Now Invert the result: clz will count *down* from the msb to the lsb, so the msb index is 31
    // and the lsb index is 0. The result for the index when counting up: msb index is 0 (because it
    // starts there), and the lsb index is 31.
    result ^= sizeof(std::uint32_t) * 8 - 1;
    *index = (unsigned int)result;
    return (non_zero != 0);
#elif defined(__has_builtin_clz)
    int leading_zeros = __builtin_clz((unsigned int)mask);
    *index = (unsigned int)leading_zeros;
    return (mask != 0);
#else
    int leading_zeros = run_time::internal_clz((unsigned int)mask);
    *index = (unsigned int)leading_zeros;
    return (mask != 0);
#endif
}

static inline
int CountLeadingZeroBits_nonzero(std::uint32_t mask)
{
    assert(mask != 0);
#if defined(_MSC_VER)
    unsigned long index;
    unsigned char non_zero = ::_BitScanReverse(&index, mask);
    // Now Invert the result: clz will count *down* from the msb to the lsb, so the msb index is 31
    // and the lsb index is 0. The result for the index when counting up: msb index is 0 (because it
    // starts there), and the lsb index is 31.
    index ^= sizeof(std::uint32_t) * 8 - 1;
    (void)non_zero;
    return (int)index;
#elif defined(__has_builtin_clz)
    int leading_zeros = __builtin_clz((unsigned int)mask);
    return leading_zeros;
#else
    int leading_zeros = internal_clz((unsigned int)mask);
    return leading_zeros;
#endif
}

#if __IS_X86_64

static inline
bool CountLeadingZeroBits(std::uint64_t mask, unsigned int * index)
{
    assert(index != nullptr);
#if defined(_MSC_VER)
    unsigned long result;
    unsigned char non_zero = ::_BitScanReverse64(&result, mask);
    // Now Invert the result: clzll will count *down* from the msb to the lsb, so the msb index is 63
    // and the lsb index is 0. The result for the index when counting up: msb index is 0 (because it
    // starts there), and the lsb index is 63.
    result ^= sizeof(std::uint64_t) * 8 - 1;
    *index = (unsigned int)result;
    return (non_zero != 0);
#elif defined(__has_builtin_clzll)
    int leading_zeros = __builtin_clzll((unsigned long long)mask);
    *index = (unsigned int)leading_zeros;
    return (mask != 0);
#else
    int leading_zeros = internal_clzll(mask);
    *index = (unsigned int)leading_zeros;
    return (mask != 0);
#endif
}

static inline
int CountLeadingZeroBits_nonzero(std::uint64_t mask)
{
    assert(mask != 0);
#if defined(_MSC_VER)
    unsigned long index;
    unsigned char non_zero = ::_BitScanReverse64(&index, mask);
    // Now Invert the result: clzll will count *down* from the msb to the lsb, so the msb index is 63
    // and the lsb index is 0. The result for the index when counting up: msb index is 0 (because it
    // starts there), and the lsb index is 63.
    index ^= sizeof(std::uint64_t) * 8 - 1;
    (void)non_zero;
    return (int)index;
#elif defined(__has_builtin_clzll)
    int leading_zeros = __builtin_clzll((unsigned long long)mask);
    return leading_zeros;
#else
    int leading_zeros = internal_clzll(mask);
    return leading_zeros;
#endif
}

#endif // __amd64__

//////////////////////////////////////////////////////////////

template <typename SizeType>
inline bool is_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: is_pow2(SizeType n) -- SizeType must be a integral type.");
    typedef typename std::make_unsigned<SizeType>::type unsigned_type;
    unsigned_type u = static_cast<unsigned_type>(n);
    return ((u & (u - 1)) == 0);
}

template <typename SizeType>
inline SizeType verify_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: verify_pow2(SizeType n) -- SizeType must be a integral type.");
    typedef typename std::make_unsigned<SizeType>::type unsigned_type;
    unsigned_type u = static_cast<unsigned_type>(n);
    return static_cast<SizeType>(u & (u - 1));
}

#if JSTD_SUPPORT_X86_BITSCAN_INSTRUCTION

static inline
std::size_t round_down_pow2(std::size_t n)
{
    if (n <= 1) {
        return 0;
    }

    unsigned int index;
    unsigned char nonZero = run_time::BitScanReverse(n - 1, &index);
    return (std::size_t(1) << index);
}

static inline
std::size_t round_to_pow2(std::size_t n)
{
    unsigned int index;
    unsigned char nonZero = run_time::BitScanReverse(n, &index);
    return (nonZero ? (std::size_t(1) << index) : std::size_t(0));
}

static inline
std::size_t round_up_pow2(std::size_t n)
{
    if (run_time::is_pow2(n)) {
        return n;
    }

    if (n <= ((std::numeric_limits<std::size_t>::max)() / 2 + 1)) {
        unsigned int index;
        unsigned char nonZero = run_time::BitScanReverse(n - 1, &index);
        return (nonZero ? (std::size_t(1) << (index + 1)) : std::size_t(0));
    }
    else {
        return (std::numeric_limits<std::size_t>::max)();
    }
}

static inline
std::size_t next_pow2(std::size_t n)
{
    if (n < ((std::numeric_limits<std::size_t>::max)() / 2 + 1)) {
        unsigned int index;
        unsigned char nonZero = BitScanReverse(n, &index);
        return (nonZero ? (std::size_t(1) << (index + 1)) : std::size_t(1));
    }
    else {
        return (std::numeric_limits<std::size_t>::max)();
    }
}

#else // !JSTD_SUPPORT_X86_BITSCAN_INSTRUCTION

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4293)
#endif

//
// N is power of 2, and [x] is rounding function.
//
// If n is power of 2:
//      N = 2 ^ [Log2(n)],
//
// If n is not power of 2:
//      (2 ^ [Log2(n) - 1]) < N <= 2 ^ [Log2(n)]
//
template <typename SizeType>
inline SizeType round_to_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: round_to_pow2(SizeType n) -- SizeType must be a integral type.");
    typedef typename std::make_unsigned<SizeType>::type unsigned_type;
    unsigned_type u = static_cast<unsigned_type>(n);
    if (is_pow2(u)) {
        return static_cast<SizeType>(u);
    }
    else {
        u = static_cast<unsigned_type>(u - 1);
        u = u | (u >> 1);
        u = u | (u >> 2);
        u = u | (u >> 4);
        u = u | (u >> 8);
        u = u | (u >> 16);
        if (jstd::integral_traits<unsigned_type>::bits >= 64) {
            u = u | (u >> 32);
        }
        if (jstd::integral_traits<unsigned_type>::bits >= 128) {
            u = u | (u >> 64);
        }
        return (u != jstd::integral_traits<unsigned_type>::max_num) ?
                static_cast<SizeType>((u + 1) / 2) :
                static_cast<SizeType>(jstd::integral_traits<unsigned_type>::max_power2);
    }
}

//
// N is power of 2, and [x] is rounding function.
//
//   (2 ^ [Log2(n) - 1]) <= N < (2 ^ [Log2(n)])
//
template <typename SizeType>
inline SizeType round_down_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: round_down_pow2(SizeType n) -- SizeType must be a integral type.");
    if (n != 0)
        return round_to_pow2<SizeType>(n - 1);
    else
        return 0;
}

//
// N is power of 2, and [x] is rounding function.
//
// If n is power of 2:
//      N = 2 ^ [Log2(n)],
//
// If n is not power of 2:
//      2 ^ [Log2(n)] < N <= 2 ^ ([Log2(n)] + 1)
//
template <typename SizeType>
inline SizeType round_up_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: round_up_pow2(SizeType n) -- SizeType must be a integral type.");
    typedef typename std::make_unsigned<SizeType>::type unsigned_type;
    unsigned_type u = static_cast<unsigned_type>(n);
    if (is_pow2(u)) {
        return static_cast<SizeType>(u);
    }
    else {
        u = static_cast<unsigned_type>(u - 1);
        u = u | (u >> 1);
        u = u | (u >> 2);
        u = u | (u >> 4);
        u = u | (u >> 8);
        u = u | (u >> 16);
        if (jstd::integral_traits<unsigned_type>::bits >= 64) {
            u = u | (u >> 32);
        }
        if (jstd::integral_traits<unsigned_type>::bits >= 128) {
            u = u | (u >> 64);
        }
        return (u != jstd::integral_traits<unsigned_type>::max_num) ?
                static_cast<SizeType>(u + 1) : static_cast<SizeType>(u);
    }
}

//
// N is power of 2, and [x] is rounding function.
//
//   2 ^ [Log2(n)] < N <= 2 ^ ([Log2(n)] + 1)
//
template <typename SizeType>
inline SizeType next_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: next_pow2(SizeType n) -- SizeType must be a integral type.");
    if (n < (std::numeric_limits<SizeType>::max)())
        return round_up_pow2<SizeType>(n + 1);
    else
        return n;
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif // JSTD_SUPPORT_X86_BITSCAN_INSTRUCTION

} // namespace run_time
} // namespace jstd

#endif // JSTD_SUPPORT_RT_POWEROF2_H