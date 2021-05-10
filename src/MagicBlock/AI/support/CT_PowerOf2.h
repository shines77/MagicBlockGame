
#ifndef JSTD_SUPPORT_CT_POWEROF2_H
#define JSTD_SUPPORT_CT_POWEROF2_H

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

namespace jstd {

template <typename T>
struct integral_traits {
    typedef typename std::make_signed<T>::type      signed_type;
    typedef typename std::make_unsigned<T>::type    unsigned_type;

    static_assert(std::is_integral<T>::value,
        "Error: jstd::integral_traits<T> -- T must be a integral type.");

    // Bits
    static const size_t bytes = sizeof(T);
    static const size_t bits = bytes * 8;
    static const size_t max_shift = bits - 1;

    // 0xFFFFFFFFUL;
    static const unsigned_type max_num = static_cast<unsigned_type>(-1);
    // 0x80000000UL;
    static const unsigned_type max_power2 = static_cast<unsigned_type>(1) << max_shift;
};

} // namespace jstd

//////////////////////////////////////////////////////////////////////////
//
// Bit Twiddling Hacks
//
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
//
//////////////////////////////////////////////////////////////////////////

namespace jstd {
namespace compile_time {

//////////////////////////////////////////////////////////////////////////////////

//
// is_pow2 = (N && ((N & (N - 1)) == 0);
// Here, N must be a unsigned number.
//
template <std::size_t N>
struct is_pow2 {
    static const bool value = ((N & (N - 1)) == 0);
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_to_pow2<N>

template <std::size_t N, std::size_t Power2>
struct round_to_pow2_impl {
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t max_power2 = jstd::integral_traits<std::size_t>::max_power2;
    static const std::size_t next_power2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N > max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const std::size_t value = ((N >= max_power2) ? max_power2 :
           (((Power2 == max_power2) || (Power2 >= N)) ? (Power2 / 2) :
            round_to_pow2_impl<N, next_power2>::value));
};

template <std::size_t N>
struct round_to_pow2_impl<N, std::size_t(0)> {
    static const std::size_t max_power2 = jstd::integral_traits<std::size_t>::max_power2;
    static const std::size_t value = max_power2;
};

template <std::size_t N>
struct round_to_pow2 {
    static const std::size_t value = (is_pow2<N>::value ? N : round_to_pow2_impl<N, 1>::value);
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_down_pow2<N>

template <std::size_t N>
struct round_down_pow2 {
    static const std::size_t value = (N != 0) ? round_to_pow2<N - 1>::value : 0;
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_up_pow2<N>

template <std::size_t N, std::size_t Power2>
struct round_up_pow2_impl {
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t max_power2 = jstd::integral_traits<std::size_t>::max_power2;
    static const std::size_t next_power2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N >= max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const std::size_t value = ((N > max_power2) ? max_num :
           (((Power2 == max_power2) || (Power2 >= N)) ? Power2 :
            round_up_pow2_impl<N, next_power2>::value));
};

template <std::size_t N>
struct round_up_pow2_impl<N, std::size_t(0)> {
    static const std::size_t value = jstd::integral_traits<std::size_t>::max_num;
};

template <std::size_t N>
struct round_up_pow2 {
    static const std::size_t value = is_pow2<N>::value ? N : round_up_pow2_impl<N, 1>::value;
};

//////////////////////////////////////////////////////////////////////////////////

// struct next_pow2<N>

template <std::size_t N, std::size_t Power2>
struct next_pow2_impl {
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t max_power2 = jstd::integral_traits<std::size_t>::max_power2;
    static const std::size_t next_power2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N >= max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const std::size_t value = ((N >= max_power2) ? max_num :
           (((Power2 == max_power2) || (Power2 > N)) ? Power2 :
            next_pow2_impl<N, next_power2>::value));
};

template <std::size_t N>
struct next_pow2_impl<N, std::size_t(0)> {
    static const std::size_t value = jstd::integral_traits<std::size_t>::max_num;
};

template <std::size_t N>
struct next_pow2 {
    static const std::size_t value = next_pow2_impl<N, 1>::value;
};

template <>
struct next_pow2<std::size_t(0)> {
    static const std::size_t value = 1;
};

//////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4307)
#endif

//////////////////////////////////////////////////////////////////////////////////

// struct round_to_power2_impl<N>

template <std::size_t N>
struct round_to_power2_impl {
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t max_power2 = jstd::integral_traits<std::size_t>::max_power2;
    static const std::size_t N1 = N - 1;
    static const std::size_t N2 = N1 | (N1 >> 1);
    static const std::size_t N3 = N2 | (N2 >> 2);
    static const std::size_t N4 = N3 | (N3 >> 4);
    static const std::size_t N5 = N4 | (N4 >> 8);
    static const std::size_t N6 = N5 | (N5 >> 16);
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    static const std::size_t N7 = N6 | (N6 >> 32);
    static const std::size_t value = (N7 != max_num) ? ((N7 + 1) / 2) : max_power2;
#else
    static const std::size_t value = (N6 != max_num) ? ((N6 + 1) / 2) : max_power2;
#endif
};

template <std::size_t N>
struct round_to_power2 {
    static const std::size_t value = is_pow2<N>::value ? N : round_to_power2_impl<N>::value;
};

// struct round_down_power2<N>

template <std::size_t N>
struct round_down_power2 {
    static const std::size_t value = (N != 0) ? round_to_power2<N - 1>::value : 0;
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_up_power2<N>

template <std::size_t N>
struct round_up_power2_impl {
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t N1 = N - 1;
    static const std::size_t N2 = N1 | (N1 >> 1);
    static const std::size_t N3 = N2 | (N2 >> 2);
    static const std::size_t N4 = N3 | (N3 >> 4);
    static const std::size_t N5 = N4 | (N4 >> 8);
    static const std::size_t N6 = N5 | (N5 >> 16);
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    static const std::size_t N7 = N6 | (N6 >> 32);
    static const std::size_t value = (N7 != max_num) ? (N7 + 1) : max_num;
#else
    static const std::size_t value = (N6 != max_num) ? (N6 + 1) : max_num;
#endif
};

template <std::size_t N>
struct round_up_power2 {
    static const std::size_t value = is_pow2<N>::value ? N : round_up_power2_impl<N>::value;
};

//////////////////////////////////////////////////////////////////////////////////

// struct next_power2<N>

template <std::size_t N>
struct next_power2 {
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t value = (N < max_num) ? round_up_power2<N + 1>::value : max_num;
};

//////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

//////////////////////////////////////////////////////////////////////////////////

// struct CountLeadingZeroBits<N>

template <std::size_t Bits, std::size_t N>
struct CountLeadingZeroBits_impl {
    static const std::size_t value = (N == 0) ? Bits : CountLeadingZeroBits_impl<Bits + 1, N * 2>::value;
};

#if 1
//
// CountLeadingZeroBits_impl<0, 0> = 0
//
template <std::size_t Bits>
struct CountLeadingZeroBits_impl<Bits, std::size_t(0)> {
    static const std::size_t value = (Bits > 0) ? (Bits - 1) : 0;
};

#else

#define COUNT_LEADING_ZERO_BITS_IMPL(bits) \
    template <> \
    struct CountLeadingZeroBits_impl<std::size_t(bits), std::size_t(0)> { \
        static const std::size_t value = (bits > 0) ? (bits - 1) : 0; \
    }

COUNT_LEADING_ZERO_BITS_IMPL(64);
COUNT_LEADING_ZERO_BITS_IMPL(63);
COUNT_LEADING_ZERO_BITS_IMPL(62);
COUNT_LEADING_ZERO_BITS_IMPL(61);
COUNT_LEADING_ZERO_BITS_IMPL(60);

COUNT_LEADING_ZERO_BITS_IMPL(59);
COUNT_LEADING_ZERO_BITS_IMPL(58);
COUNT_LEADING_ZERO_BITS_IMPL(57);
COUNT_LEADING_ZERO_BITS_IMPL(56);
COUNT_LEADING_ZERO_BITS_IMPL(55);
COUNT_LEADING_ZERO_BITS_IMPL(54);
COUNT_LEADING_ZERO_BITS_IMPL(53);
COUNT_LEADING_ZERO_BITS_IMPL(52);
COUNT_LEADING_ZERO_BITS_IMPL(51);
COUNT_LEADING_ZERO_BITS_IMPL(50);

COUNT_LEADING_ZERO_BITS_IMPL(49);
COUNT_LEADING_ZERO_BITS_IMPL(48);
COUNT_LEADING_ZERO_BITS_IMPL(47);
COUNT_LEADING_ZERO_BITS_IMPL(46);
COUNT_LEADING_ZERO_BITS_IMPL(45);
COUNT_LEADING_ZERO_BITS_IMPL(44);
COUNT_LEADING_ZERO_BITS_IMPL(43);
COUNT_LEADING_ZERO_BITS_IMPL(42);
COUNT_LEADING_ZERO_BITS_IMPL(41);
COUNT_LEADING_ZERO_BITS_IMPL(40);

COUNT_LEADING_ZERO_BITS_IMPL(39);
COUNT_LEADING_ZERO_BITS_IMPL(38);
COUNT_LEADING_ZERO_BITS_IMPL(37);
COUNT_LEADING_ZERO_BITS_IMPL(36);
COUNT_LEADING_ZERO_BITS_IMPL(35);
COUNT_LEADING_ZERO_BITS_IMPL(34);
COUNT_LEADING_ZERO_BITS_IMPL(33);
COUNT_LEADING_ZERO_BITS_IMPL(32);
COUNT_LEADING_ZERO_BITS_IMPL(31);
COUNT_LEADING_ZERO_BITS_IMPL(30);

COUNT_LEADING_ZERO_BITS_IMPL(29);
COUNT_LEADING_ZERO_BITS_IMPL(28);
COUNT_LEADING_ZERO_BITS_IMPL(27);
COUNT_LEADING_ZERO_BITS_IMPL(26);
COUNT_LEADING_ZERO_BITS_IMPL(25);
COUNT_LEADING_ZERO_BITS_IMPL(24);
COUNT_LEADING_ZERO_BITS_IMPL(23);
COUNT_LEADING_ZERO_BITS_IMPL(22);
COUNT_LEADING_ZERO_BITS_IMPL(21);
COUNT_LEADING_ZERO_BITS_IMPL(20);

COUNT_LEADING_ZERO_BITS_IMPL(19);
COUNT_LEADING_ZERO_BITS_IMPL(18);
COUNT_LEADING_ZERO_BITS_IMPL(17);
COUNT_LEADING_ZERO_BITS_IMPL(16);
COUNT_LEADING_ZERO_BITS_IMPL(15);
COUNT_LEADING_ZERO_BITS_IMPL(14);
COUNT_LEADING_ZERO_BITS_IMPL(13);
COUNT_LEADING_ZERO_BITS_IMPL(12);
COUNT_LEADING_ZERO_BITS_IMPL(11);
COUNT_LEADING_ZERO_BITS_IMPL(10);

COUNT_LEADING_ZERO_BITS_IMPL(9);
COUNT_LEADING_ZERO_BITS_IMPL(8);
COUNT_LEADING_ZERO_BITS_IMPL(7);
COUNT_LEADING_ZERO_BITS_IMPL(6);
COUNT_LEADING_ZERO_BITS_IMPL(5);
COUNT_LEADING_ZERO_BITS_IMPL(4);
COUNT_LEADING_ZERO_BITS_IMPL(3);
COUNT_LEADING_ZERO_BITS_IMPL(2);
COUNT_LEADING_ZERO_BITS_IMPL(1);
COUNT_LEADING_ZERO_BITS_IMPL(0);
#endif

template <std::size_t N>
struct CountLeadingZeroBits {
    static const std::size_t round_down_2 = is_pow2<N>::value ? N : round_down_pow2<N>::value;
    static const std::size_t value = CountLeadingZeroBits_impl<0, round_down_2>::value;
};

//////////////////////////////////////////////////////////////////////////////////

// struct CountTrailingZeroBits<N>

template <std::size_t Bits, std::size_t N>
struct CountTrailingZeroBits_impl {
    static const std::size_t value = (N == 0) ? Bits : CountTrailingZeroBits_impl<Bits + 1, N / 2>::value;
};

#if 1
//
// CountTrailingZeroBits<0, 0> = 64
//
template <std::size_t Bits>
struct CountTrailingZeroBits_impl<Bits, std::size_t(0)> {
    static const std::size_t value = (Bits > 0) ? (Bits - 1) : (sizeof(std::size_t) * 8);
};

#else

#define COUNT_TRAILING_ZERO_BITS_IMPL(bits) \
    template <> \
    struct CountTrailingZeroBits_impl<std::size_t(bits), std::size_t(0)> { \
        static const std::size_t value = (bits > 0) ? (bits - 1) : (sizeof(std::size_t) * 8); \
    }

COUNT_TRAILING_ZERO_BITS_IMPL(64);
COUNT_TRAILING_ZERO_BITS_IMPL(63);
COUNT_TRAILING_ZERO_BITS_IMPL(62);
COUNT_TRAILING_ZERO_BITS_IMPL(61);
COUNT_TRAILING_ZERO_BITS_IMPL(60);

COUNT_TRAILING_ZERO_BITS_IMPL(59);
COUNT_TRAILING_ZERO_BITS_IMPL(58);
COUNT_TRAILING_ZERO_BITS_IMPL(57);
COUNT_TRAILING_ZERO_BITS_IMPL(56);
COUNT_TRAILING_ZERO_BITS_IMPL(55);
COUNT_TRAILING_ZERO_BITS_IMPL(54);
COUNT_TRAILING_ZERO_BITS_IMPL(53);
COUNT_TRAILING_ZERO_BITS_IMPL(52);
COUNT_TRAILING_ZERO_BITS_IMPL(51);
COUNT_TRAILING_ZERO_BITS_IMPL(50);

COUNT_TRAILING_ZERO_BITS_IMPL(49);
COUNT_TRAILING_ZERO_BITS_IMPL(48);
COUNT_TRAILING_ZERO_BITS_IMPL(47);
COUNT_TRAILING_ZERO_BITS_IMPL(46);
COUNT_TRAILING_ZERO_BITS_IMPL(45);
COUNT_TRAILING_ZERO_BITS_IMPL(44);
COUNT_TRAILING_ZERO_BITS_IMPL(43);
COUNT_TRAILING_ZERO_BITS_IMPL(42);
COUNT_TRAILING_ZERO_BITS_IMPL(41);
COUNT_TRAILING_ZERO_BITS_IMPL(40);

COUNT_TRAILING_ZERO_BITS_IMPL(39);
COUNT_TRAILING_ZERO_BITS_IMPL(38);
COUNT_TRAILING_ZERO_BITS_IMPL(37);
COUNT_TRAILING_ZERO_BITS_IMPL(36);
COUNT_TRAILING_ZERO_BITS_IMPL(35);
COUNT_TRAILING_ZERO_BITS_IMPL(34);
COUNT_TRAILING_ZERO_BITS_IMPL(33);
COUNT_TRAILING_ZERO_BITS_IMPL(32);
COUNT_TRAILING_ZERO_BITS_IMPL(31);
COUNT_TRAILING_ZERO_BITS_IMPL(30);

COUNT_TRAILING_ZERO_BITS_IMPL(29);
COUNT_TRAILING_ZERO_BITS_IMPL(28);
COUNT_TRAILING_ZERO_BITS_IMPL(27);
COUNT_TRAILING_ZERO_BITS_IMPL(26);
COUNT_TRAILING_ZERO_BITS_IMPL(25);
COUNT_TRAILING_ZERO_BITS_IMPL(24);
COUNT_TRAILING_ZERO_BITS_IMPL(23);
COUNT_TRAILING_ZERO_BITS_IMPL(22);
COUNT_TRAILING_ZERO_BITS_IMPL(21);
COUNT_TRAILING_ZERO_BITS_IMPL(20);

COUNT_TRAILING_ZERO_BITS_IMPL(19);
COUNT_TRAILING_ZERO_BITS_IMPL(18);
COUNT_TRAILING_ZERO_BITS_IMPL(17);
COUNT_TRAILING_ZERO_BITS_IMPL(16);
COUNT_TRAILING_ZERO_BITS_IMPL(15);
COUNT_TRAILING_ZERO_BITS_IMPL(14);
COUNT_TRAILING_ZERO_BITS_IMPL(13);
COUNT_TRAILING_ZERO_BITS_IMPL(12);
COUNT_TRAILING_ZERO_BITS_IMPL(11);
COUNT_TRAILING_ZERO_BITS_IMPL(10);

COUNT_TRAILING_ZERO_BITS_IMPL(9);
COUNT_TRAILING_ZERO_BITS_IMPL(8);
COUNT_TRAILING_ZERO_BITS_IMPL(7);
COUNT_TRAILING_ZERO_BITS_IMPL(6);
COUNT_TRAILING_ZERO_BITS_IMPL(5);
COUNT_TRAILING_ZERO_BITS_IMPL(4);
COUNT_TRAILING_ZERO_BITS_IMPL(3);
COUNT_TRAILING_ZERO_BITS_IMPL(2);
COUNT_TRAILING_ZERO_BITS_IMPL(1);
COUNT_TRAILING_ZERO_BITS_IMPL(0);
#endif

template <std::size_t N>
struct CountTrailingZeroBits {
    static const std::size_t round_up_2 = is_pow2<N>::value ? N : round_up_pow2<N>::value;
    static const std::size_t value = CountTrailingZeroBits_impl<0, round_up_2>::value;
};

//////////////////////////////////////////////////////////////////////////////////

} // namespace compile_time
} // namespace jstd

#endif // JSTD_SUPPORT_CT_POWEROF2_H
