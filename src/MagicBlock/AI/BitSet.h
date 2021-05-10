
#ifndef JSTD_BITSET_H
#define JSTD_BITSET_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstring>          // For std::memset()
#include <initializer_list>
#include <type_traits>
#include <algorithm>        // For std::swap(), until C++11, For std::min()
#include <utility>          // For std::swap(), since C++11
#include <cassert>

#include "MagicBlock/AI/BitUtils.h"

namespace jstd {

struct dont_init_t {};

template <size_t Size, size_t Alignment>
struct AlignedTo {
    static const size_t alignment = Alignment;
    static const size_t size = Size;
    static const size_t value = (size + alignment - 1) / alignment * alignment;
};

template <size_t Bits>
class BitSet {
public:
    typedef typename std::conditional<
                (Bits <= sizeof(uint32_t) * 8), uint32_t, size_t
            >::type  unit_type;
    typedef BitSet<Bits> this_type;

    static const size_t kUnitBytes = sizeof(unit_type);
    static const size_t kUnitBits  = 8 * kUnitBytes;

    static const size_t kUnits = (Bits + kUnitBits - 1) / kUnitBits;
    static const size_t kBits  = kUnits * kUnitBits;
    static const size_t kBytes = kUnits * kUnitBytes;
    static const size_t kRestBits = (Bits % kUnitBits);
    static const unit_type kFullMask = unit_type(-1);
    static const unit_type kTrimMask = (kRestBits != 0) ? (unit_type(size_t(1) << kRestBits) - 1) : kFullMask;

protected:
    unit_type * array_;

    void allocate_array() {
        this->array_ = new unit_type[kUnits];
    }

    void destory_array() {
        if (this->array_) {
            delete[] this->array_;
            this->array_ = nullptr;
        }
    }

public:
    BitSet() noexcept : array_(nullptr)  {
        static_assert((Bits != 0), "BitSet<Bits>: Bits can not be 0 size.");
        this->allocate_array();
        this->reset();
    }

    BitSet(dont_init_t & dont_init) noexcept : array_(nullptr) {
        static_assert((Bits != 0), "BitSet<Bits>: Bits can not be 0 size.");
        /* Here we don't need initialize for optimize sometimes. */
    }

    BitSet(const this_type & src) noexcept : array_(nullptr) {
        static_assert((Bits != 0), "PackedBitSet<Bits>: Bits can not be 0 size.");
        this->allocate_array();
        for (size_t i = 0; i < kUnits; i++) {
            this->array_[i] = src.array(i);
        }
    }

    template <size_t UBits>
    BitSet(const BitSet<UBits> & src) noexcept : array_(nullptr) {
        static_assert((Bits != 0), "PackedBitSet<Bits>: Bits can not be 0 size.");
        typedef BitSet<UBits> SourceBitMap;
        static const size_t copyUnits = std::min(kUnits, SourceBitMap::kUnits);
        this->allocate_array();
        for (size_t i = 0; i < copyUnits; i++) {
            this->array_[i] = src.array(i);
        }
        if (kRestBits != 0) {
            this->trim();
        }
    }

    BitSet(this_type && src) noexcept : array_(nullptr) {
        static_assert((Bits != 0), "PackedBitSet<Bits>: Bits can not be 0 size.");
        assert (this != &src);
        std::swap(this->array_, src.array_);
    }

    BitSet(unit_type value) noexcept : array_(nullptr) {
        static_assert((Bits != 0), "PackedBitSet<Bits>: Bits can not be 0 size.");
        this->allocate_array();
        if (kRestBits == 0)
            this->array_[0] = value;
        else
            this->array_[0] = value & kTrimMask;
    }

    BitSet(std::initializer_list<unit_type> init_list) noexcept : array_(nullptr) {
        static_assert((Bits != 0), "PackedBitSet<Bits>: Bits can not be 0 size.");
        this->allocate_array();
        if (init_list.size() <= kUnits) {
            size_t i = 0;
            for (auto iter : init_list) {
                this->array_[i++] = *iter;
            }
            if (kRestBits != 0) {
                this->trim();
            }
        }
        else {
            size_t i = 0;
            for (auto iter : init_list) {
                this->array_[i++] = *iter;
                if (i >= kUnits) {
                    break;
                }
            }
            if (kRestBits != 0) {
                this->trim();
            }
        }
    }

    ~BitSet() {
        this->destory_array();
    }

    size_t size() const        { return Bits; }

          char * data()        { return (char *)      this->array_; }
    const char * data() const  { return (const char *)this->array_; }

    size_t total_bytes() const { return kBytes;     }
    size_t unit_size() const   { return kUnits;     }
    size_t unit_bits() const   { return kUnitBits;  }
    size_t unit_bytes() const  { return kUnitBytes; }

    void swap(this_type & src) noexcept {
        if (this != &src) {
            std::swap(this->array_, src.array_);
        }
    }

    unit_type array(size_t index) const {
        assert(index < kUnits);
        return this->array_[index];
    }

    this_type & init(std::initializer_list<unit_type> init_list) noexcept {
        if (init_list.size() <= kUnits) {
            size_t i = 0;
            for (auto iter : init_list) {
                this->array_[i++] = *iter;
            }
            if (kRestBits != 0) {
                this->trim();
            }
        }
        else {
            size_t i = 0;
            for (auto iter : init_list) {
                this->array_[i++] = *iter;
                if (i >= kUnits) {
                    break;
                }
            }
            if (kRestBits != 0) {
                this->trim();
            }
        }
        return (*this);
    }

    class reference {
    private:
        this_type * bitset_;    // pointer to the bitmap
        size_t pos_;            // position of element in bitset

        // proxy for an element
        friend class BitSet<Bits>;

    public:
        ~reference() noexcept {
            // destroy the object
        }

        reference & operator = (bool value) noexcept {
            // assign Boolean to element
            this->bitset_->set(pos_, value);
            return (*this);
        }

        reference & operator = (const reference & right) noexcept {
            // assign reference to element
            this->bitset_->set(pos_, bool(right));
            return (*this);
        }

        reference & flip() noexcept {
            // complement stored element
            this->bitset_->flip(pos_);
            return (*this);
        }

        bool operator ~ () const noexcept {
            // return complemented element
            return (!this->bitset_->test(pos_));
        }

        bool operator ! () const noexcept {
            // return complemented element
            return (!this->bitset_->test(pos_));
        }

        operator bool () const noexcept {
            // return element
            return (this->bitset_->test(pos_));
        }

    private:
        reference() noexcept
            : bitset_(nullptr), pos_(0) {
            // default construct
        }

        reference(this_type & bitsets, size_t pos) noexcept
            : bitset_(&bitsets), pos_(pos) {
            // construct from bitmap reference and position
        }
    };

    this_type & operator = (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            this->array_[i] = right.array(i);
        }
        return (*this);
    }

    constexpr bool operator [] (size_t pos) const {
        assert(pos < Bits);
        return this->test(pos);
    }

	reference operator [] (size_t pos) {
        assert(pos < Bits);
        return reference(*this, pos);
    }

    this_type & operator & (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] &= value;
        else
            this->array_[0] &= value & kTrimMask;
        return (*this);
    }

    this_type & operator | (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] |= value;
        else
            this->array_[0] |= value & kTrimMask;
        return (*this);
    }

    this_type & operator ^ (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] ^= value;
        else
            this->array_[0] ^= value & kTrimMask;
        return (*this);
    }

    this_type & operator &= (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] &= value;
        else
            this->array_[0] &= value & kTrimMask;
        return (*this);
    }

    this_type & operator |= (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] |= value;
        else
            this->array_[0] |= value & kTrimMask;
        return (*this);
    }

    this_type & operator ^= (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] ^= value;
        else
            this->array_[0] ^= value & kTrimMask;
        return (*this);
    }

    this_type & operator &= (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            this->array_[i] &= right.array(i);
        }
        return (*this);
    }

    this_type & operator |= (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            this->array_[i] |= right.array(i);
        }
        return (*this);
    }

    this_type & operator ^= (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            this->array_[i] ^= right.array(i);
        }
        return (*this);
    }

	this_type operator ~ () const noexcept {
        // Flip all bits
		return (this_type(*this).flip());
    }

	this_type operator ! () const noexcept {
        // Flip all bits
		return (this_type(*this).flip());
    }

    bool operator == (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            if (this->array_[i] != right.array(i)) {
                return false;
            }
        }
        return true;
    }

    bool operator != (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            if (this->array_[i] != right.array(i)) {
                return true;
            }
        }
        return false;
    }

    this_type & fill(size_t value) noexcept {
        if (kRestBits != 0) {
            size_t i = 0;
            for (; i < kUnits - 1; i++) {
                this->array_[i] = (unit_type)value;
            }
            this->array_[i] = (unit_type)value & kTrimMask;
        }
        else {
            for (size_t i = 0; i < kUnits; i++) {
                this->array_[i] = (unit_type)value;
            }
        }
        return (*this);
    }

    this_type & set() noexcept {
        if (kRestBits != 0) {
            if (kUnits <= 8) {
                size_t i = 0;
                for (; i < kUnits - 1; i++) {
                    this->array_[i] = kFullMask;
                }
                this->array_[i] = kTrimMask;
            }
            else {
                std::memset(this->array_, (kFullMask & 0xFF), (kUnits - 1) * sizeof(unit_type));
                this->array_[kUnits - 1] = kTrimMask;
            }
        }
        else {
            if (kUnits <= 8) {
                for (size_t i = 0; i < kUnits; i++) {
                    this->array_[i] = kFullMask;
                }
            }
            else {
                std::memset(this->array_, (kFullMask & 0xFF), kUnits * sizeof(unit_type));
            }
        }
        return (*this);
    }

    this_type & set(size_t pos) {
        assert(pos < Bits);
        if (Bits <= kUnitBits) {
            this->array_[0] |= unit_type(size_t(1) << pos);
        }
        else {
            size_t index = pos / kUnitBits;
            size_t shift = pos % kUnitBits;
            this->array_[index] |= unit_type(size_t(1) << shift);
        }
        return (*this);
    }

    this_type & set(size_t pos, bool value) {
        if (value)
            this->set(pos);
        else
            this->reset(pos);
        return (*this);
    }

    this_type & set(size_t index, size_t pos) {
        assert(index < kUnits);
        assert(pos < kUnitBits);
        this->array_[index] |= unit_type(size_t(1) << pos);
        return (*this);
    }

    this_type & set_bit(size_t index, size_t bit) {
        assert(index < kUnits);
        this->array_[index] |= unit_type(bit);
        return (*this);
    }

    this_type & set(size_t index, size_t pos, bool value) {
        if (value)
            this->set(index, pos);
        else
            this->reset(index, pos);
        return (*this);
    }

    this_type & reset() noexcept {
        if (kUnits <= 8) {
            for (size_t i = 0; i < kUnits; i++) {
                this->array_[i] = 0;
            }
        }
        else {
            std::memset(this->array_, 0, kUnits * sizeof(unit_type));
        }
        return (*this);
    }

#if 1
    this_type & reset(size_t pos) {
        assert(pos < Bits);
        if (Bits <= kUnitBits) {
            this->array_[0] ^= unit_type(size_t(1) << pos);
        }
        else {
            size_t index = pos / kUnitBits;
            size_t shift = pos % kUnitBits;
            this->array_[index] ^= unit_type(size_t(1) << shift);
        }
        return (*this);
    }

    this_type & reset(size_t index, size_t pos) {
        assert(index < kUnits);
        assert(pos < kUnitBits);
        this->array_[index] ^= unit_type(size_t(1) << pos);
        return (*this);
    }

    this_type & reset_bit(size_t index, size_t bit) {
        assert(index < kUnits);
        this->array_[index] ^= unit_type(bit);
        return (*this);
    }
#else
    this_type & reset(size_t pos) {
        assert(pos < Bits);
        if (Bits <= kUnitBits) {
            this->array_[0] &= unit_type(~(size_t(1) << pos));
        }
        else {
            size_t index = pos / kUnitBits;
            size_t shift = pos % kUnitBits;
            this->array_[index] &= unit_type(~(size_t(1) << shift));
        }
        return (*this);
    }

    this_type & reset(size_t index, size_t pos) {
        assert(index < kUnits);
        assert(pos < kUnitBits);
        this->array_[index] &= unit_type(~(size_t(1) << pos));
        return (*this);
    }

    this_type & reset_bit(size_t index, size_t bit) {
        assert(index < kUnits);
        this->array_[index] &= unit_type(~bit);
        return (*this);
    }
#endif

    this_type & flip() noexcept {
        if (kRestBits != 0) {
            size_t i = 0;
            for (; i < kUnits - 1; i++) {
                this->array_[i] ^= kFullMask;
            }
            this->array_[i] ^= kTrimMask;
        }
        else {
            for (size_t i = 0; i < kUnits; i++) {
                this->array_[i] ^= kFullMask;
            }
        }
        return (*this);
    }

    this_type & flip(size_t pos) {
        assert(pos < Bits);
        if (Bits <= kUnitBits) {
            this->array_[0] ^= unit_type(~(size_t(1) << pos));
        }
        else {
            size_t index = pos / kUnitBits;
            size_t shift = pos % kUnitBits;
            this->array_[index] ^= unit_type(~(size_t(1) << shift));
        }
        return (*this);
    }

    this_type & flip(size_t index, size_t pos) {
        assert(index < kUnits);
        assert(pos < kUnitBits);
        this->array_[index] ^= unit_type(~(size_t(1) << pos));
        return (*this);
    }

    this_type & flip_bit(size_t index, size_t bit) {
        assert(index < kUnits);
        this->array_[index] ^= unit_type(~bit);
        return (*this);
    }

    this_type & trim() noexcept {
        if (kRestBits != 0) {
		    this->array_[kUnits - 1] &= kTrimMask;
        }
        return (*this);
    }

    bool test(size_t pos) const {
        assert(pos < Bits);
        if (Bits <= kUnitBits) {
            return ((this->array_[0] & unit_type(size_t(1) << pos)) != 0);
        }
        else {
            size_t index = pos / kUnitBits;
            size_t shift = pos % kUnitBits;
            return ((this->array_[index] & unit_type(size_t(1) << shift)) != 0);
        }
    }

    bool test(size_t index, size_t pos) const {
        assert(index < kUnits);
        assert(pos < kUnitBits);
        return ((this->array_[index] & unit_type(size_t(1) << pos)) != 0);
    }

    bool test_bit(size_t index, size_t bit) const {
        assert(index < kUnits);
        return ((this->array_[index] & unit_type(bit)) != 0);
    }

    size_t value(size_t pos) const {
        assert(pos < Bits);
        if (Bits <= kUnitBits) {
            return size_t(this->array_[0]);
        }
        else {
            size_t index = pos / kUnitBits;
            return size_t(this->array_[index]);
        }
    }

    unit_type get_uint(size_t index) const {
        assert(index < kUnits);
        return this->array_[index];
    }

    void set_uint(size_t index, unit_type value) const {
        assert(index < kUnits);
        return this->array_[index] = value;
    }

    bool any() const noexcept {
        for (size_t i = 0; i < kUnits - 1; i++) {
            unit_type unit = this->array_[i];
            if (unit != 0) {
                return true;
            }
        }
        return (this->array_[kUnits - 1] != 0);
    }

    bool none() const noexcept {
#if 1
        return !(this->any());
#else
        for (size_t i = 0; i < kUnits - 1; i++) {
            unit_type unit = this->array_[i];
            if (unit != 0) {
                return false;
            }
        }
        return (this->array_[kUnits - 1] == 0);
#endif
    }

    bool all() const noexcept {
        for (size_t i = 0; i < kUnits - 1; i++) {
            unit_type unit = this->array_[i];
            if (unit != kFullMask) {
                return false;
            }
        }
        if (kRestBits != 0) {
            unit_type unit = this->array_[kUnits - 1] & kTrimMask;
            return (unit == kTrimMask);
        }
        else {
            return (this->array_[kUnits - 1] == kFullMask);
        }
    }

    size_t count() const noexcept {
        size_t total_popcnt = 0;
        for (size_t i = 0; i < kUnits; i++) {
            size_t unit = this->array_[i];
            unsigned int popcnt = BitUtils::popcnt<Bits>(unit);
            total_popcnt += popcnt;
        }
        return total_popcnt;
    }

    unsigned long to_ulong() const {
        if (Bits <= sizeof(uint32_t) * 8) {
            return static_cast<unsigned long>(this->array_[0]);
        }
        else {
            return static_cast<unsigned long>(this->array_[0]);
        }
    }

    uint64_t to_ullong() const {
        if (Bits <= sizeof(uint32_t) * 8) {
            return static_cast<uint64_t>(this->array_[0]);
        }
        else {
            return this->array_[0];
        }
    }

    unit_type value() const {
        return this->array_[0];
    }

    size_t value_sz() const {
        if (Bits <= sizeof(uint32_t) * 8) {
            return static_cast<size_t>(this->array_[0]);
        }
        else {
            return this->array_[0];
        }
    }
};

template <size_t Bits>
inline
BitSet<Bits> operator & (const BitSet<Bits> & left,
                         const BitSet<Bits> & right) noexcept {
    // left And right
    BitSet<Bits> answer = left;
    answer &= right;
    return std::move(answer);
}

template <size_t Bits>
inline
BitSet<Bits> operator | (const BitSet<Bits> & left,
                         const BitSet<Bits> & right) noexcept {
    // left Or right
    BitSet<Bits> answer = left;
    answer |= right;
    return std::move(answer);
}

template <size_t Bits>
inline
BitSet<Bits> operator ^ (const BitSet<Bits> & left,
                         const BitSet<Bits> & right) noexcept {
    // left Xor right
    BitSet<Bits> answer = left;
    answer ^= right;
    return std::move(answer);
}

} // namespace jstd

#endif // JSTD_BITSET_H
