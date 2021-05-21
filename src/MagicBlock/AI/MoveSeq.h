#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>  // For std::forward<T>

namespace MagicBlock {
namespace AI {

#pragma pack(push, 1)

//
// Move sequense for direction [Up, Down, Left, Right]
//
class MoveSeq {
public:
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  ssize_type;

    typedef std::size_t     unit_type;
    typedef MoveSeq         this_type;

    static const size_type kBitsPerMove = 2;
    static const size_type kMoveMask = (size_type(1) << kBitsPerMove) - 1;
    static const size_type kBytesPerUnit= sizeof(unit_type);
    static const size_type kBitsPerUnit= sizeof(unit_type) * 8;
    static const size_type kSizesPerUnit= kBitsPerUnit/ kBitsPerMove;
    static const size_type kInnerThresholdSize = kBitsPerUnit/ kBitsPerMove;

    static const size_type EmptyMove = size_type(-1);

private:
    std::uint8_t    size_;
    unit_type       seq_;

private:
    void alloc_outer_data(size_type new_size) {
        size_type unit_size = (new_size + kSizesPerUnit- 1) / kSizesPerUnit;
        assert(unit_size > 1);
        unit_type * new_units = new unit_type[unit_size];
        if (unit_size == 2) {
            // Copy old inner seq to outer buffer
            new_units[0] = this->seq_;
            new_units[1] = 0;
            this->seq_ = (size_type)new_units;
        }
        else {
            if (unit_size <= 1) {
                throw std::runtime_error("Exception: MoveSeq::alloc_outer_data(): unit_size <= 1;");
            }
            // Copy old outer buffer to new buffer
            const unit_type * old_units = this->data();
            size_type i = 0;
            for (; i < unit_size - 1; i++) {
                new_units[i] = old_units[i];
            }
            // Clear and reset the new allocte unit
            new_units[i] = 0;

            delete[] old_units;
            this->seq_ = (size_type)new_units;
        }
    }

    void copy_outer_data(const MoveSeq & other) {
        size_type unit_size = other.unit_capacity();
        assert(unit_size > 1);
        unit_type * new_units = new unit_type[unit_size];
        std::memcpy(new_units, other.data(), unit_size);
        for (size_type i = 0; i < unit_size; i++) {
            new_units[i] = other.units(i);
        }
        this->seq_ = (size_type)new_units;
    }

    void copy_data(const MoveSeq & other) {
        if (this->is_inner())
            this->seq_ = other.seq_;
        else
            this->copy_outer_data(other);
    }

    void internal_copy(const MoveSeq & other) {
        this->size_ = other.size_;
        this->copy_data(other);
    }

    void internal_shadow_copy(const MoveSeq & other) {
        this->size_ = other.size_;
        this->seq_  = other.seq_;
    }

    void internal_move(MoveSeq && other) {
        this->internal_shadow_copy(other);
        other.reset();
    }

    void internal_swap(MoveSeq & other) {
        std::swap(this->size_, other.size_);
        std::swap(this->seq_,  other.seq_);
    }

public:
    MoveSeq() : size_(0), seq_(0) {}
    MoveSeq(const MoveSeq & src) {
        this->internal_copy(src);
    }
    MoveSeq(MoveSeq && src) : size_(src.size_), seq_(src.seq_) {
        this->reset();
    }
    ~MoveSeq() {
        this->destroy();
    }

    void destroy() {
        if (!this->is_inner()) {
            unit_type * ptr = (unit_type *)this->seq_;
            if (ptr != nullptr) {
                delete[] ptr;
                this->seq_ = 0;
            }
            this->size_ = 0;
        }
    }

    size_type size() const {
        return this->size_;
    }

    size_type capacity() const {
        return (this->size() + kSizesPerUnit- 1) / kSizesPerUnit* kSizesPerUnit;
    }

    size_type unit_capacity() const {
        return (this->size() + kSizesPerUnit- 1) / kSizesPerUnit;
    }

    bool empty() const {
        return (this->size_ == 0);
    }

    unit_type * data() {
        return (unit_type *)this->seq_;
    }

    const unit_type * data() const {
        return (const unit_type *)this->seq_;
    }

    unit_type units(size_type index) const {
        assert(index < this->unit_capacity());
        return *(this->data() + index);
    }

    unit_type inner_seq() const {
        return this->seq_;
    }

    unit_type * outer_seq() {
        return this->data();
    }

    const unit_type * outer_seq() const {
        return this->data();
    }

    bool is_inner() const {
        return (this->size_ <= kInnerThresholdSize);
    }

    bool is_inner(size_type _size) const {
        return (_size <= kInnerThresholdSize);
    }

    bool is_overflow() const {
        return (this->size_ >= kInnerThresholdSize);
    }

    MoveSeq & operator = (const MoveSeq & rhs) {
        this->copy(rhs);
        return *this;
    }

    MoveSeq & operator = (MoveSeq && rhs) {
        this->move(std::forward<MoveSeq>(rhs));
        return *this;
    }

    std::uint8_t operator [] (size_type pos) const {
        return (std::uint8_t)this->get_move(pos);
    }

    // Dangerous: This function only used to move assignment constructor / operator.
    void reset() {
        this->size_ = 0;
        this->seq_  = 0;
    }

    void clear() {
        if (!this->is_inner()) {
            unit_type * ptr = (unit_type *)this->seq_;
            if (ptr != nullptr) {
                delete[] ptr;
                this->seq_ = 0;
            }
        }
        this->size_ = 0;
    }

    void copy(const MoveSeq & other) {
        if (&other != this) {
            this->internal_copy(other);
        }
    }

    void move(MoveSeq && other) {
        if (&other != this) {
            this->internal_move(std::forward<MoveSeq>(other));
        }
    }

    void swap(MoveSeq & other) {
        if (&other != this) {
            this->internal_swap(other);
        }
    }

    size_type get_move(size_type pos) const {
        assert(pos < this->size());
        if (this->is_inner()) {
            size_type u_pos = pos;
            size_type mv_shift = u_pos * kBitsPerMove;
            size_type mv = size_type((this->seq_ >> mv_shift) & kMoveMask);
            return mv;
        }
        else {
            size_type u_index = pos / kSizesPerUnit;
            size_type u_pos   = pos % kSizesPerUnit;
            size_type mv_shift = u_pos * kBitsPerMove;
            const unit_type * units = this->data();
            size_type mv = size_type((units[u_index] >> mv_shift) & kMoveMask);
            return mv;
        }
    }

    void set_move(size_type pos, size_type move) {
        assert(pos < this->size());
        size_type mv = move & kMoveMask;
        if (this->is_inner()) {
            size_type u_pos = pos;
            this->seq_ |= unit_type(mv << (u_pos * kBitsPerMove));
        }
        else {
            size_type u_index = pos / kSizesPerUnit;
            size_type u_pos   = pos % kSizesPerUnit;
            unit_type * units = this->data();
            units[u_index] |= unit_type(mv << (u_pos * kBitsPerMove));
        }
    }

    size_type grow_up(ssize_type delta) {
        return size_type(this->size_ + delta);
    }

    size_type front() const {
        if (this->size() > 0)
            return this->get_move(0);
        else
            return this_type::EmptyMove;
    }

    void push_back(size_type move) {
        size_type mv = move & kMoveMask;
        if (this->size() < kInnerThresholdSize) {
            size_type pos = this->size();
            size_type mv_mask = kMoveMask << (pos * kBitsPerMove);
            assert((this->seq_ & mv_mask) == 0);
            (void)mv_mask;
            this->seq_ |= unit_type(mv << (pos * kBitsPerMove));
            assert(this->size() < 255);
            this->size_++;
        }
        else {
            size_type index = this->size() / kSizesPerUnit;
            size_type pos   = this->size() % kSizesPerUnit;
            if (pos == 0) {
                size_type new_size = this->grow_up(1);
                this->alloc_outer_data(new_size);
            }
            unit_type * units = this->data();
            size_type mv_mask = kMoveMask << (pos * kBitsPerMove);
            assert((units[index] & mv_mask) == 0);
            (void)mv_mask;
            units[index] |= unit_type(mv << (pos * kBitsPerMove));
            assert(this->size() < 255);
            this->size_++;
        }
    }

    size_type back() const {
        if (this->size() > 0)
            return this->get_move(this->size() - 1);
        else
            return this_type::EmptyMove;
    }

    void pop_back() {
        assert(this->size() >= 0);
        size_type new_size = this->grow_up(-1);
        if (new_size == kInnerThresholdSize) {
            unit_type * old_units = this->data();
            assert(old_units != nullptr);
            this->seq_ = old_units[0];
            delete[] old_units;
        }
        this->size_ = std::uint8_t(new_size);
    }
};

#pragma pack(pop)

} // namespace AI
} // namespace MagicBlock
