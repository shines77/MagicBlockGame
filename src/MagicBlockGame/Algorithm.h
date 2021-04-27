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

namespace MagicBlock {
namespace Algorithm {

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
static int binary_search(std::uint16_t * ptr, std::size_t first,
                         std::size_t last, std::uint16_t value)
{
    std::size_t low = first;
    std::size_t high = last;

    //while (std::ptrdiff_t(high - low) >= 8) {
    while (low < high) {
        std::size_t mid = (low + high) / 2;
        std::uint16_t middle = ptr[mid];
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
    return -1;
}
#else
static int binary_search(std::uint16_t * ptr, std::size_t first,
                         std::size_t last, std::uint16_t value)
{
    std::size_t low = first;
    std::size_t high = last;

    assert(low < high);
    std::uint16_t maximum = ptr[high - 1];
    if (value < maximum) {
        std::uint16_t minimum = ptr[low];
        if (value > minimum) {
            //while (std::ptrdiff_t(high - low) >= 8) {
            while (low < high) {
                std::size_t mid = (low + high) / 2;
                std::uint16_t middle = ptr[mid];
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
