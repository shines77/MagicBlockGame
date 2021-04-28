#pragma once

#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <malloc.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>

#include <cstring>
#include <memory>
#include <algorithm>        // For std::swap(), until C++11
#include <utility>          // For std::swap(), since C++11
#include <exception>
#include <stdexcept>

#include <immintrin.h>
#include <emmintrin.h>

//#define __SSE2__
//#define __AVX2__

namespace MagicBlock {
namespace Algorithm {

#ifdef __SSE2__

static inline
__m128i _mm_loadu_esi16(short value)
{
    int value32 = (int)value;
    __m128 value128_ps = _mm_loadu_ps((float const *)&value32);
    __m128i value128 = _mm_load_si128((__m128i const *)&value128_ps);
    return value128;
}

static inline
__m128i _mm_loadu_esi32(int value)
{
    __m128 value128_ps = _mm_loadu_ps((float const *)&value);
    __m128i value128 = _mm_load_si128((__m128i const *)&value128_ps);
    return value128;
}

static inline
__m128i _mm_loadu_esi64(int64_t value)
{
    __m128d value128_pd = _mm_loadu_pd((double const *)&value);
    __m128i value128 = _mm_load_si128((__m128i const *)&value128_pd);
    return value128;
}

#endif // __SSE2__

static int find_uint16(std::uint16_t * buf, std::size_t len, std::uint16_t value)
{
    std::uint16_t * indexFirst = buf;
    std::uint16_t * indexLast  = buf + len;
    for (std::uint16_t * indexs = indexFirst; indexs < indexLast; indexs++) {
        assert(*indexs != std::uint16_t(-1));
        if (*indexs != value)
            continue;
        else
            return int(indexs - buf);
    }
    return -1;
}

static int find_uint16(std::uint16_t * buf, std::size_t first, std::size_t last, std::uint16_t value)
{
    std::uint16_t * indexFirst = buf + first;
    std::uint16_t * indexLast  = buf + last;
    for (std::uint16_t * indexs = indexFirst; indexs < indexLast; indexs++) {
        assert(*indexs != std::uint16_t(-1));
        if (*indexs != value)
            continue;
        else
            return int(indexs - buf);
    }
    return -1;
}

//
// See: https://software.intel.com/sites/landingpage/IntrinsicsGuide/
// See: https://www.felixcloutier.com/x86/punpcklbw:punpcklwd:punpckldq:punpcklqdq
//
static int find_uint16_sse2(std::uint16_t * buf, std::size_t first,
                            std::size_t last, std::uint16_t value)
{
#ifdef __SSE2__
    static const std::size_t kXMMAlignment = 16;
    static const std::size_t kXMMAlignMask = kXMMAlignment - 1;

    static const std::size_t kSingelStepSize = sizeof(__m128i) / sizeof(std::uint16_t);
    static const std::size_t kCmpEqualStep = 2 * kSingelStepSize;

    std::ptrdiff_t len = last - first;
    assert(len > 0);

    std::uint16_t * buf_start = buf + first;
    std::uint16_t * aligned_start;
    if (len <= 64)
        aligned_start = buf + last;
    else
        aligned_start = (std::uint16_t *)((std::size_t)buf_start & (~kXMMAlignMask));

    for (std::uint16_t * indexs = buf_start; indexs < aligned_start; indexs++) {
        if (*indexs != value)
            continue;
        else
            return int(indexs - buf);
    }

    if (len <= 64) {
        return -1;
    }

#if 1
    __m128i value128 = _mm_set1_epi16(value);
#else
    __m128i value32 = _mm_loadu_esi16(value);
    value32 = _mm_unpacklo_epi16(value32, value32);
    value32 = _mm_unpacklo_epi32(value32, value32);
    value32 = _mm_shuffle_epi32(value32, 0);
    __m128i value128 = value32;
#endif

    std::uint16_t * current = aligned_start;
    std::uint16_t * aligned_end = (std::uint16_t *)((std::size_t)(buf + last) & (~kXMMAlignMask));

    while ((current + kCmpEqualStep) <= aligned_end) {
        __m128i index128_0 = _mm_load_si128((__m128i const *)current + 0);
        __m128i index128_1 = _mm_load_si128((__m128i const *)current + 1);
        __m128i mask128_0 = _mm_cmpeq_epi16(index128_0, value128);
        __m128i mask128_1 = _mm_cmpeq_epi16(index128_1, value128);
        int mask32_0 = _mm_movemask_epi8(mask128_0);
        int mask32_1 = _mm_movemask_epi8(mask128_1);
        if (mask32_0 != 0) {
            unsigned long index;
            unsigned char non_zero = _BitScanForward(&index, mask32_0);
            (void)non_zero;
            return (int)((current - buf) + index / 2);
        }
        else if (mask32_1 != 0) {
            unsigned long index;
            unsigned char non_zero = _BitScanForward(&index, mask32_1);
            (void)non_zero;
            return (int)((current - buf) + kSingelStepSize + index / 2);
        }
        current += kCmpEqualStep;
    }

    std::uint16_t * buf_end = buf + last;
    for (std::uint16_t * indexs = current; indexs < buf_end; indexs++) {
        if (*indexs != value)
            continue;
        else
            return int(indexs - buf);
    }

    return -1;
#else
    return find_uint16(buf, first, last, value);
#endif // __SSE2__
}

static int find_uint16_sse2(std::uint16_t * buf, std::size_t len, std::uint16_t value) {
    return find_uint16_sse2(buf, 0, len, value);
}

static int find_uint16_sse2_has_bug(std::uint16_t * buf, std::size_t len, std::uint16_t value)
{
#ifdef __SSE2__
    static const std::size_t kXMMAlignment = 16;
    static const std::size_t kXMMAlignMask = kXMMAlignment - 1;

    static const std::size_t kSingelStepSize = sizeof(__m128i) / sizeof(std::uint16_t);
    static const std::size_t kCmpEqualStep = 2 * kSingelStepSize;

    if (len < 64) {
        return find_uint16(buf, 0, len, value);
    }

    std::uint16_t * aligned_start = (std::uint16_t *)((std::size_t)buf & (~kXMMAlignMask));

    for (std::uint16_t * indexs = buf; indexs < aligned_start; indexs++) {
        if (*indexs != value)
            continue;
        else
            return int(indexs - buf);
    }

#ifdef __AVX2__
    __m128i value128_0 = _mm_set1_epi16(value);
    __m128i value128_1 = _mm_set1_epi16(value);
#else
    __m128i value32 = _mm_loadu_esi16(value);
    value32 = _mm_unpacklo_epi16(value32, value32);
    value32 = _mm_unpacklo_epi32(value32, value32);
    value32 = _mm_shuffle_epi32(value32, 0);
    __m128i value128_0 = value32;
    __m128i value128_1 = value32;
#endif // __AVX2__

    std::uint16_t * current = aligned_start;
    std::uint16_t * aligned_end = (std::uint16_t *)((std::size_t)(buf + len) & (~kXMMAlignMask));

    while ((current + kCmpEqualStep) < aligned_end) {
        __m128i index128_0 = _mm_load_si128((__m128i const *)current + 0);
        __m128i index128_1 = _mm_load_si128((__m128i const *)current + 1);
        __m128i mask128_0 = _mm_cmpeq_epi16(index128_0, value128_0);
        __m128i mask128_1 = _mm_cmpeq_epi16(index128_1, value128_1);
        int mask32_0 = _mm_movemask_epi8(mask128_0);
        int mask32_1 = _mm_movemask_epi8(mask128_1);
        if (mask32_0 != 0) {
            unsigned long index;
            unsigned char non_zero = _BitScanForward(&index, mask32_0);
            (void)non_zero;
            return (int)((current - aligned_start) + index / 2);
        }
        else if (mask32_1 != 0) {
            unsigned long index;
            unsigned char non_zero = _BitScanForward(&index, mask32_1);
            (void)non_zero;
            return (int)((current - aligned_start) + kSingelStepSize + index / 2);
        }
        current += kCmpEqualStep;
    }

    std::uint16_t * buf_end = buf + len;
    for (std::uint16_t * indexs = aligned_end; indexs < buf_end; indexs++) {
        if (*indexs != value)
            continue;
        else
            return int(indexs - aligned_end);
    }

    return -1;
#else
    return find_uint16(buf, len, value);
#endif // __SSE2__
}

static void merge_sort(std::uint16_t * indexs, std::uint16_t * new_indexs,
                       std::size_t first, std::size_t last)
{
    assert(indexs != nullptr);
    assert(new_indexs != nullptr);
    assert(first < last);
    std::size_t left = first;
    std::size_t middle = (first + last) / 2;
    std::size_t right = middle;
    std::size_t cur = first;
    while (left < middle && right < last) {
        if (indexs[left] <= indexs[right])
            new_indexs[cur++] = indexs[left++];
        else
            new_indexs[cur++] = indexs[right++];
    }

    while (left < middle) {
        new_indexs[cur++] = indexs[left++];
    }

    while (right < last) {
        new_indexs[cur++] = indexs[right++];
    }
}

static void merge_sort(std::uint16_t * indexs, std::uintptr_t ** values,
                       std::uint16_t * new_indexs, std::uintptr_t ** new_values,
                       std::size_t first, std::size_t last)
{
    assert(indexs != nullptr);
    assert(new_indexs != nullptr);
    assert(values != nullptr);
    assert(new_values != nullptr);
    assert(first < last);
    std::size_t left = first;
    std::size_t middle = (first + last) / 2;
    std::size_t right = middle;
    std::size_t cur = first;
    while (left < middle && right < last) {
        if (indexs[left] <= indexs[right]) {
            new_indexs[cur] = indexs[left];
            new_values[cur] = values[left];
            cur++;
            left++;
        }
        else {
            new_indexs[cur] = indexs[right];
            new_values[cur] = values[right];
            cur++;
            right++;
        }
        assert(cur <= last);
    }

    while (left < middle) {
        new_indexs[cur] = indexs[left];
        new_values[cur] = values[left];
        cur++;
        left++;
        assert(cur <= last);
    }

    while (right < last) {
        new_indexs[cur] = indexs[right];
        new_values[cur] = values[right];
        cur++;
        right++;
        assert(cur <= last);
    }
}

#if 1
//
// See: https://www.cnblogs.com/skywang12345/p/3596746.html
// See: https://blog.csdn.net/zuiaituantuan/article/details/5978009 (recommend)
//

// Prerequisite: all elements are unique
static void quick_sort(std::uint16_t * indexs, std::ptrdiff_t first, std::ptrdiff_t last)
{
    assert(indexs != nullptr);
    if (first < last) {
        std::ptrdiff_t left = first;
        std::ptrdiff_t right = last;
        std::uint16_t pivot = indexs[left];
        while (left < right) {
            while (left < right && indexs[right] > pivot) {
                right--;
            }
            if (left < right) {
                indexs[left++] = indexs[right];
            }
            while (left < right && indexs[left] < pivot) {
                left++;
            }
            if (left < right) {
                indexs[right--] = indexs[left];
            }
        }
        indexs[left] = pivot;

        quick_sort(indexs, first, left - 1);
        quick_sort(indexs, left + 1, last);
    }
}

// Prerequisite: all elements are unique
static void quick_sort(std::uint16_t * indexs, std::uintptr_t ** values,
                       std::ptrdiff_t first, std::ptrdiff_t last)
{
    assert(indexs != nullptr);
    assert(values != nullptr);
    if (first < last) {
        std::ptrdiff_t left = first;
        std::ptrdiff_t right = last;
        std::uint16_t pivot = indexs[left];
        std::uintptr_t * pivot_value = values[left];
        while (left < right) {
            while (left < right && indexs[right] > pivot) {
                right--;
            }
            if (left < right) {
                indexs[left] = indexs[right];
                values[left] = values[right];
                left++;
            }
            while (left < right && indexs[left] < pivot) {
                left++;
            }
            if (left < right) {
                indexs[right] = indexs[left];
                values[right] = values[left];
                right--;
            }
        }
        indexs[left] = pivot;
        values[left] = pivot_value;

        quick_sort(indexs, values, first, left - 1);
        quick_sort(indexs, values, left + 1, last);
    }
}
#else
//
// See: https://baijiahao.baidu.com/s?id=1667443239764190065
// See: https://blog.csdn.net/zuiaituantuan/article/details/5978009 (recommend)
//

// Prerequisite: all elements are unique
static void quick_sort(std::uint16_t * indexs, std::ptrdiff_t first, std::ptrdiff_t last)
{
    assert(indexs != nullptr);
    if (first < last) {
        std::ptrdiff_t left = first;
        std::ptrdiff_t right = last;
        std::size_t middle = (left + right) / 2;
        std::swap(indexs[left], indexs[middle]);
        std::uint16_t pivot = indexs[left];
        while (left < right) {
            while (left < right && indexs[right] > pivot) {
                right--;
            }
            if (left < right) {
                indexs[left++] = indexs[right];
            }
            while (left < right && indexs[left] < pivot) {
                left++;
            }
            if (left < right) {
                indexs[right--] = indexs[left];
            }
        }
        indexs[left] = pivot;

        quick_sort(indexs, first, left - 1);
        quick_sort(indexs, left + 1, last);
    }
}

// Prerequisite: all elements are unique
static void quick_sort(std::uint16_t * indexs, std::uintptr_t ** values,
                       std::ptrdiff_t first, std::ptrdiff_t last)
{
    assert(indexs != nullptr);
    assert(values != nullptr);
    if (first < last) {
        std::ptrdiff_t left = first;
        std::ptrdiff_t right = last;
        std::ptrdiff_t middle = (left + right) / 2;
        std::swap(indexs[left], indexs[middle]);
        std::swap(values[left], values[middle]);
        std::uint16_t pivot = indexs[left];
        std::uintptr_t * pivot_value = values[left];
        while (left < right) {
            while (left < right && indexs[right] > pivot) {
                right--;
            }
            if (left < right) {
                indexs[left] = indexs[right];
                values[left] = values[right];
                left++;
            }
            while (left < right && indexs[left] < pivot) {
                left++;
            }
            if (left < right) {
                indexs[right] = indexs[left];
                values[right] = values[left];
                right--;
            }
        }
        indexs[left] = pivot;
        values[left] = pivot_value;

        quick_sort(indexs, values, first, left - 1);
        quick_sort(indexs, values, left + 1, last);
    }
}
#endif

#if 1
static int binary_search(std::uint16_t * buf, std::size_t first,
                         std::size_t last, std::uint16_t value)
{
    std::size_t low = first;
    std::size_t high = last;

    //while (std::ptrdiff_t(high - low) >= 64) {
    while (low < high) {
        std::size_t mid = (low + high) / 2;
        std::uint16_t middle = buf[mid];
        if (value < middle)
            high = mid;
        else if (value > middle)
            low = mid + 1;
        else
            return (int)mid;
    }

    return -1;

#ifdef __SSE2__
    if (low < high)
        return Algorithm::find_uint16_sse2(buf, low, high, value);
    else
        return -1;
#else
    std::uint16_t * indexFirst = (std::uint16_t *)buf + low;
    std::uint16_t * indexLast  = (std::uint16_t *)buf + high;
    for (std::uint16_t * indexs = indexFirst; indexs < indexLast; indexs++) {
        assert(*indexs != std::uint16_t(-1));
        if (*indexs != value)
            continue;
        else
            return int(indexs - (std::uint16_t *)buf);
    }
    return -1;
#endif
}
#else
static int binary_search(std::uint16_t * buf, std::size_t first,
                         std::size_t last, std::uint16_t value)
{
    std::size_t low = first;
    std::size_t high = last;

    assert(low < high);
    std::uint16_t maximum = buf[high - 1];
    if (value < maximum) {
        std::uint16_t minimum = buf[low];
        if (value > minimum) {
            //while (std::ptrdiff_t(high - low) >= 8) {
            while (low < high) {
                std::size_t mid = (low + high) / 2;
                std::uint16_t middle = buf[mid];
                if (value < middle)
                    high = mid;
                else if (value > middle)
                    low = mid + 1;
                else
                    return (int)mid;
            }
#if 0
            std::uint16_t * indexFirst = (std::uint16_t *)ptr + low;
            std::uint16_t * indexLast  = (std::uint16_t *)ptr + high;
            for (std::uint16_t * indexs = indexFirst; indexs < indexLast; indexs++) {
                assert(*indexs != std::uint16_t(-1));
                if (*indexs != value)
                    continue;
                else
                    return int(indexs - (std::uint16_t *)ptr);
            }
#endif
        }
        else if (value == minimum) {
            return (int)low;
        }
    }
    else if (value == maximum) {
        return (int)(high - 1);
    }
    return -1;
}
#endif

} // namespace Algorithm
} // namespace MagicBlock
