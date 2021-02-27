
#ifndef JSTD_BITUTILS_H
#define JSTD_BITUTILS_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>

namespace jstd {

struct BitUtils {
    template <size_t Bits>
    static unsigned int popcnt(size_t n) noexcept {
		const char * const _BitsPerByte =
			"\0\1\1\2\1\2\2\3\1\2\2\3\2\3\3\4"
			"\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
			"\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
			"\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
			"\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
			"\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
			"\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
			"\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
			"\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
			"\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
			"\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
			"\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
			"\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
			"\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
			"\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
			"\4\5\5\6\5\6\6\7\5\6\6\7\6\7\7\x8";

        unsigned int pop_count;
        if (Bits <= 8) {
            pop_count = _BitsPerByte[n & 0xFFUL];
        }
        else if (Bits <= 16) {
            pop_count = _BitsPerByte[n & 0xFFUL] + _BitsPerByte[(n >> 8) & 0xFFUL];
        }
        else if (Bits <= 24) {
            pop_count = _BitsPerByte[n         & 0xFFUL] + _BitsPerByte[(n >> 8) & 0xFFUL] +
                        _BitsPerByte[(n >> 16) & 0xFFUL];
        }
        else if (Bits <= 32) {
            pop_count = _BitsPerByte[n         & 0xFFUL] + _BitsPerByte[(n >> 8)  & 0xFFUL] +
                        _BitsPerByte[(n >> 16) & 0xFFUL] + _BitsPerByte[(n >> 24) & 0xFFUL];
        }
        else if (Bits <= 40) {
            pop_count = _BitsPerByte[n         & 0xFFUL] + _BitsPerByte[(n >> 8)  & 0xFFUL] +
                        _BitsPerByte[(n >> 16) & 0xFFUL] + _BitsPerByte[(n >> 24) & 0xFFUL] +
                        _BitsPerByte[(n >> 32) & 0xFFUL];
        }
        else if (Bits <= 48) {
            pop_count = _BitsPerByte[n         & 0xFFUL] + _BitsPerByte[(n >> 8)  & 0xFFUL] +
                        _BitsPerByte[(n >> 16) & 0xFFUL] + _BitsPerByte[(n >> 24) & 0xFFUL] +
                        _BitsPerByte[(n >> 32) & 0xFFUL] + _BitsPerByte[(n >> 40) & 0xFFUL];
        }
        else if (Bits <= 56) {
            pop_count = _BitsPerByte[n         & 0xFFUL] + _BitsPerByte[(n >> 8)  & 0xFFUL] +
                        _BitsPerByte[(n >> 16) & 0xFFUL] + _BitsPerByte[(n >> 24) & 0xFFUL] +
                        _BitsPerByte[(n >> 32) & 0xFFUL] + _BitsPerByte[(n >> 40) & 0xFFUL] +
                        _BitsPerByte[(n >> 48) & 0xFFUL];
        }
        else {
            pop_count = _BitsPerByte[n         & 0xFFUL] + _BitsPerByte[(n >> 8)  & 0xFFUL] +
                        _BitsPerByte[(n >> 16) & 0xFFUL] + _BitsPerByte[(n >> 24) & 0xFFUL] +
                        _BitsPerByte[(n >> 32) & 0xFFUL] + _BitsPerByte[(n >> 40) & 0xFFUL] +
                        _BitsPerByte[(n >> 48) & 0xFFUL] + _BitsPerByte[(n >> 56) & 0xFFUL];
        }
        return pop_count;
    }
};

} // namespace jstd

#endif // JSTD_BITUTILS_H
