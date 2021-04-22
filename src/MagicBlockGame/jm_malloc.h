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

#include "support/CT_PowerOf2.h"

namespace jm_malloc {

#pragma pack(push, 1)

template <std::size_t PoolId = 0>
class ThreadMalloc {
public:
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  ssize_type;

    typedef ThreadMalloc<PoolId> malloc_type;

    static const size_type kChunkPageSize = 4096;
    static const size_type kChunkAlignment = 4096;

    // The maximum chunk unit size is 65536.
    static const size_type kDefaultChunkUnitSize = 65536;
    static const size_type kDefaultChunkUintBytes = 4;

    static const size_type kChunkUintBytes = jstd::compile_time::round_up_pow2<kDefaultChunkUintBytes>::value;
    static const size_type kRoundChunkUnitSize = jstd::compile_time::round_up_pow2<kDefaultChunkUnitSize>::value;
    static const size_type kChunkUnitSize = (kRoundChunkUnitSize <= 65536) ? kRoundChunkUnitSize : 65536; 

    static const size_type kChunkLowShift = jstd::compile_time::CountTrailingZeroBits<kChunkUnitSize>::value;
    static const size_type kChunkLowMask = kChunkUnitSize - 1;

    //
    // The chunk size is set to 128 KB for faster memory allocation.
    // Default value is 64KB * 4 = 256KB.
    //
    static const size_type kChunkTotalBytes = kChunkUnitSize * kChunkUintBytes;

    class PtrHandle {
    protected:
        std::uint32_t value_;

    public:
        PtrHandle() : value_(0) {}
        PtrHandle(std::uint32_t value) : value_(value) {}
        PtrHandle(const PtrHandle & src) : value_(src.value_) {}
        ~PtrHandle() = default;

        std::uint32_t value() const { return this->value_; }

        template <typename U>
        U * ptr() const {
            malloc_type & malloc = malloc_type::getInstance();
            return malloc.realPtr<U>(this->value_);
        }

        void * void_ptr() const {
            return this->ptr<void>();
        }
    };

    class Chunk {
    public:
        typedef std::size_t size_type;

    protected:
        void *      chunk_;
        size_type   size_;

        void init() {
#if defined(_MSC_VER)
            void * new_chunk = ::_aligned_malloc(kChunkUnitSize * kChunkUintBytes, kChunkAlignment);
#else
            // Note: Use posix_memalign() in BSD.
            void * new_chunk = ::memalign(kChunkAlignment, kChunkSize * kUintBytes);
#endif
            if (new_chunk != nullptr) {
                this->chunk_ = new_chunk;
            }
            else {
                throw std::bad_alloc();
            }
            this->size_ = kChunkUnitSize;
        }

    public:
        Chunk(void * chunk = nullptr) : chunk_(chunk), size_(0) {
            this->init();
        }

        Chunk(const Chunk & src) = delete;

        Chunk(Chunk && src) : chunk_(nullptr), size_(0) {
            std::swap(this->chunk_, src.chunk_);
            std::swap(this->size_, src.size_);
        }

        virtual ~Chunk() {
            this->destroy();
        }

        void destroy() {
            if (this->chunk_ != nullptr) {
#if defined(_MSC_VER)
                ::_aligned_free(this->chunk_);
#else
                ::free(this->chunk_);
#endif
                this->chunk_ = nullptr;
            }
            this->size_ = 0;
        }

        void swap(Chunk & other) {
            if (&other != this) {
                std::swap(this->chunk_, other.chunk_);
                std::swap(this->size_, other.size_);
            }
        }

        template <typename U>
        U * valueOf(std::uint16_t offset) const {
            assert(this->chunk_ != nullptr);
            return reinterpret_cast<U *>(&reinterpret_cast<size_type *>(this->chunk_)[offset]);
        }

        template <typename U>
        U * valueOf(size_type offset) const {
            return this->valueOf<U>(static_cast<std::uint16_t>(offset));
        }
    }; // Chunk

    struct SizeInfo {
        std::uint16_t index;
    };

    //
    // TinyObject
    //
    // Class0: 4,
    // Class1: (interval = 8,   count = 8 ) 8, 16, 24, 32, 40, 48, 56, 64,
    // Class2: (interval = 16,  count = 12) 80, 96, 112, 128, ..., 248, 256,
    // Class3: (interval = 32,  count = 24) 288, 320, 352, 384, ..., 992, 1024,
    // Class4: (interval = 64,  count = 48) 1088, 1152, ..., 4032, 4096,
    //
    // SamllObject
    //
    // Class5: (interval = 128, count = 96)  4224, 4352, ..., 16252, 16384,
    //
    // MidimumObject
    //
    // Class6: (interval = 256, count = 192) 16640, 16896, ..., 65280, 65536,
    //
    class SizeClass {
    public:
        static const size_type kTinyObjectSizeLimit = 4096;
        static const size_type kSamllObjectSizeLimit = 16384;
        static const size_type kMidimumObjectSizeLimit = 65536;

        // count = (16384 - 4096) / 128 = 96
        static const size_type kSamllObjectSizeCount = (kSamllObjectSizeLimit - kTinyObjectSizeLimit) / 128;
        // count = (kChunkTotalBytes - 16384 + 255) / 256 = 960
        static const size_type kMidimumObjectSizeCount = (kChunkTotalBytes - kSamllObjectSizeLimit) / 256;

    private:
        std::uint16_t sizeList_[128];
        std::uint16_t sizeToIndex_[kTinyObjectSizeLimit];

        size_type maxTinyIndex_;

        void initTinyObjectSizes() {
            for (size_type i = 0; i < sizeof(this->sizeList_); i++) {
                this->sizeList_[i] = 0;
            }

            this->sizeList_[0] = 4;
            size_type index = 1;
            size_type size;
            // [8 ~ 64] (8)
            for (size = 8; size <= 64; size += 8) {
                this->sizeList_[index++] = std::uint16_t(size);
            }
            // [80 ~ 256] (12)
            for (size += 8; size <= 256; size += 16) {
                this->sizeList_[index++] = std::uint16_t(size);
            }
            // [288 ~ 1024] (24)
            for (size += 16; size <= 1024; size += 32) {
                this->sizeList_[index++] = std::uint16_t(size);
            }
            // [1088 ~ 4096) (48)
            for (size += 32; size < 4096; size += 64) {
                this->sizeList_[index++] = std::uint16_t(size);
            }

            index = 0;
            for (size_type i = 0; i < kTinyObjectSizeLimit; i++) {
                size_type alloc_size = this->sizeList_[index];
                this->sizeToIndex_[i] = std::uint16_t(index);
                if (i == alloc_size) {
                    index++;
                }
            }
            this->maxTinyIndex_ = index;
        }

    public:
        SizeClass() : maxTinyIndex_(0) {
            this->initTinyObjectSizes();
        }

        ~SizeClass() {
        }

        size_type getMaxIndex() const {
            return this->maxTinyIndex_ + kSamllObjectSizeCount + kMidimumObjectSizeCount;
        }

        size_type sizeToIndex(size_type alloc_size) const {
            size_type index;
            if (alloc_size < kTinyObjectSizeLimit) {
                // TinyObject
                // [0, 4096)
                index = this->sizeToIndex_[alloc_size];
            }
            else if (alloc_size <= kSamllObjectSizeLimit) {
                // SamllObject
                // [4096, 16384), interval = 128 bytes
                // count = (16384 - 4096) / 128 = 96
                index = this->maxTinyIndex_ + (alloc_size - kTinyObjectSizeLimit + 127) / 128ULL;
            }
            else if (alloc_size <= kChunkTotalBytes) {
                // MidimumObject
                // [16384, kChunkTotalBytes], interval = 256 bytes
                // count = (kChunkTotalBytes - 16384 + 255) / 256 = 960
                // Maximum index value = this->maxTinyIndex_(92) + 96 + 960 = 1148
                index = this->maxTinyIndex_ + kSamllObjectSizeCount + (alloc_size - kSamllObjectSizeLimit + 255) / 256ULL;
            }
            else {
                index = size_type(-1);
            }
            return index;
        }

        size_type indexToSize(size_type index) const {
            size_type alloc_size;
            if (index <= this->maxTinyIndex_) {
                // TinyObject
                // [0, 4096)
                alloc_size = this->sizeList_[index];
            }
            else if (index <= (this->maxTinyIndex_ + kSamllObjectSizeCount)) {
                // SamllObject
                // [4096, 16384), interval = 128 bytes
                alloc_size = kTinyObjectSizeLimit + (index - this->maxTinyIndex_) * 128;
            }
            else if (index <= (this->maxTinyIndex_ + kSamllObjectSizeCount + kMidimumObjectSizeCount)) {
                // MidimumObject
                // [16384, kChunkTotalBytes], interval = 256 bytes
                alloc_size = kSamllObjectSizeLimit + (index - (this->maxTinyIndex_ + kSamllObjectSizeCount)) * 256;
            }
            else {
                alloc_size = size_type(-1);
            }
            return alloc_size;
        }
    };

    class ThreadCache {
    private:
        //

    public:
        ThreadCache() {}
        ~ThreadCache() {}
    };

    typedef Chunk   chunk_type;

private:
    std::vector<chunk_type *>   chunk_list_;
    size_type                   alloc_size_;

    void init() {
        chunk_type * new_chunk = new chunk_type;
        this->chunk_list_.push_back(new_chunk);
        this->alloc_size_ = 1;
    }

public:
    ThreadMalloc() : alloc_size_(0) {
        this->init();
    }

    ThreadMalloc(const malloc_type & src) = delete;

    ThreadMalloc(malloc_type && src) : size_(0) {
        this->internal_swap(std::forward<malloc_type>(src));
    }

    virtual ~ThreadMalloc() {
        this->destroyPool();
    }

    void internal_swap(malloc_type & other) {
        std::swap(this->chunk_list_, other.chunk_list_);
        std::swap(this->alloc_size_, other.alloc_size_);
    }

    void swap(malloc_type & other) {
        if (&other != this) {
            this->internal_swap(other);
        }
    }

    size_type size() const {
        return this->chunk_list_.size();
    }

    size_type alloc_size() const {
        return this->alloc_size_;
    }

    size_type actual_size() const {
        return (this->size() * kChunkTotalBytes);
    }

    void destroyPool() {
        size_type list_size = this->chunk_list_.size();
        for (size_type i = 0; i < list_size; i++) {
            chunk_type * chunk = this->chunk_list_[i];
            if (chunk != nullptr) {
                delete this->chunk_list_[i];
                this->chunk_list_[i] = nullptr;
            }
        }
        this->chunk_list_.clear();
        this->alloc_size_ = 0;
    }

    static malloc_type & getInstance() {
        static std::map<size_type, malloc_type> malloc_pool;
        if (malloc_pool.count(PoolId) == 0) {
            malloc_pool.insert(std::make_pair(PoolId, this_type()));
        }
        return malloc_pool[PoolId];
    }

    template <typename U>
    U * realPtr(std::uint32_t handle) const {
        size_type chunk_id = (handle >> (std::uint32_t)kChunkLowShift);
        if (chunk_id < this->chunk_list_.size()) {
            chunk_type * chunk = this->chunk_list_[chunk_id];
            assert(chunk != nullptr);
            size_type offset = (handle & (std::uint32_t)kChunkLowMask);
            return chunk->valueOf<U>(offset);
        }
        return nullptr;
    }

    template <typename U>
    U * realPtr(PtrHandle handle) const {
        return this->realPtr<U>(handle.value());
    }

    static void shutdown() {
        auto & malloc = malloc_type::getInstance();
        malloc.destroyPool();
    }
};

#pragma pack(pop)

} // namespace jm_malloc
