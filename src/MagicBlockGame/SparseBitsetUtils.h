#pragma once

#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <malloc.h>

#include <cstdint>
#include <cstddef>

#include <cstring>
#include <memory>
#include <vector>
#include <map>
#include <bitset>
#include <exception>
#include <stdexcept>

#include "jm_malloc.h"

#include "Value128.h"
#include "support/CT_PowerOf2.h"

using namespace jm_malloc;

namespace MagicBlock {

#pragma pack(push, 1)

template <typename Board, std::size_t Bits, std::size_t PoolId = 0>
class SparseBitsetUtils {
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

    typedef Board                               board_type;
    typedef jm_malloc::ThreadMalloc<PoolId>     malloc_type;
    typedef typename malloc_type::Handle        Handle;

    static const size_type BoardX = board_type::Y;
    static const size_type BoardY = board_type::X;
    static const size_type BoardSize = board_type::BoardSize

private:
    size_type   alloc_size_;

    void init() {
    }

    void internal_swap(SparseBitsetUtils & other) {
        std::swap(this->alloc_size_, other.alloc_size_);
    }

public:
    SparseBitsetUtils() : alloc_size_(0) {
        this->init();
    }

    SparseBitsetUtils(const SparseBitsetUtils & src) = delete;

    SparseBitsetUtils(SparseBitsetUtils && src) : alloc_size_(0) {
        this->internal_swap(std::forward<SparseBitsetUtils>(src));
    }

    virtual ~SparseBitsetUtils() {
        this->destroy();
    }

    void swap(SparseBitsetUtils & other) {
        if (&other != this) {
            this->internal_swap(other);
        }
    }

    size_type size() const {
        return this->alloc_size_;
    }

    void destroy() {
        this->alloc_size_ = 0;
    }

    static void shutdown() {
        //
    }
};

#pragma pack(pop)

} // namespace MagicBlock
