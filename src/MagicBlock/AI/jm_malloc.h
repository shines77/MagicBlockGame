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
#include <list>
#include <map>
#include <bitset>
#include <algorithm>        // For std::swap(), until C++11
#include <utility>          // For std::swap(), since C++11
#include <exception>
#include <stdexcept>

#include "MagicBlock/AI/BitSet.h"
#include "MagicBlock/AI/support/CT_PowerOf2.h"

#define JM_MALLOC_USE_STATISTIC_INFO    1

namespace jm_malloc {

#pragma pack(push, 1)

template <std::size_t PoolId = 0>
class ThreadMalloc {
public:
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  ssize_type;

    typedef ThreadMalloc<PoolId> this_type;
    typedef ThreadMalloc<PoolId> malloc_type;

    static const size_type kChunkPageSize = 4096;
    static const size_type kChunkAlignment = 4096;

    // You can choose std::uint32_t or std::uint64_t, default is std::uint32_t.
    typedef std::uint32_t  ChunkUnitType;

    // The maximum chunk unit size is 65536. 
    // Range: [2, 65536], must be the power of 2.
    static const size_type kDefaultChunkUnitSize = 65536;
    static const size_type kChunkUintBytes = sizeof(ChunkUnitType);

    static const size_type kRoundedChunkUnitSize = jstd::compile_time::round_up_pow2<kDefaultChunkUnitSize>::value;
    static const size_type kChunkUnitSize = (kRoundedChunkUnitSize <= 65536) ? kRoundedChunkUnitSize : 65536; 

    static const size_type kChunkLowShift = jstd::compile_time::CountTrailingZeroBits<kChunkUnitSize>::value;
    static const size_type kChunkLowMask = kChunkUnitSize - 1;

    static const size_type kChunkHighCount = size_type((std::uint64_t(1) << 32) / kChunkUnitSize);
    static const size_type kChunkHighBits = sizeof(std::uint32_t) * 8 - kChunkLowShift;
    static const size_type kChunkHighMask = kChunkHighCount - 1;
    static const size_type kChunkHighMaskFull = size_type(((std::uint64_t(1) << 32) - 1) >> kChunkLowShift) << kChunkLowShift;

    //
    // The chunk size is set to 128 KB for faster memory allocation.
    // Default value is 64KB * 4 = 256KB.
    //
    static const size_type kChunkTotalBytes = kChunkUnitSize * kChunkUintBytes;

    class Handle {
    protected:
        std::uint32_t value_;

    public:
        Handle() : value_(0) {}
        Handle(std::uint32_t value) : value_(value) {}
        Handle(const Handle & src) : value_(src.value_) {}
        ~Handle() = default;

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

    // free_list<T>
    template <typename T>
    class free_list {
    public:
        typedef T                               node_type;
        typedef typename this_type::size_type   size_type;

    protected:
        node_type * next_;
        size_type   size_;

    public:
        free_list(node_type * next = nullptr) : next_(next), size_((next == nullptr) ? 0 : 1) {}
        ~free_list() {
#ifndef NDEBUG
            clear();
#endif
        }

        node_type * begin() const { return this->next_; }
        node_type * end() const   { return nullptr; }

        node_type * next() const { return this->next_; }
        size_type   size() const { return this->size_; }

        bool is_valid() const { return (this->next_ != nullptr); }
        bool is_empty() const { return (this->size_ == 0); }

        void set_next(node_type * next) {
            this->next_ = next;
        }
        void set_size(size_type size) {
            this->size_ = size;
        }

        void set_list(node_type * next, size_type size) {
            this->next_ = next;
            this->size_ = size;
        }

        void clear() {
            this->next_ = nullptr;
            this->size_ = 0;
        }

        void reset(node_type * next) {
            this->next_ = next;
            this->size_ = 0;
        }

        void increase() {
            ++(this->size_);
        }

        void decrease() {
            assert(this->size_ > 0);
            --(this->size_);
        }

        void inflate(size_type size) {
            this->size_ += size;
        }

        void deflate(size_type size) {
            assert(this->size_ >= size);
            this->size_ -= size;
        }

        void push_front(node_type * node) {
            assert(node != nullptr);
            node->next = this->next_;
            this->next_ = node;
            this->increase();
        }

        node_type * pop_front() {
            assert(this->next_ != nullptr);
            node_type * node = this->next_;
            this->next_ = node->next;
            this->decrease();
            return node;
        }

        node_type * front() {
            return this->next();
        }

        node_type * back() {
            node_type * prev = nullptr;
            node_type * node = this->next_;
            while (node != nullptr) {
                prev = node;
                node = node->next;
            }
            return prev;
        }

        void erase(node_type * where, node_type * node) {
            if (where != nullptr) {
                if (where->next == node) {
                    where->next = node->next;
                    this->decrease();
                }
            }
            else {
                if (this->next_ == node) {
                    this->next_ = node->next;
                    this->decrease();
                }
            }
        }

        void swap(free_list & other) {
            if (&other != this) {
                std::swap(this->next_, other.next_);
                std::swap(this->size_, other.size_);
            }
        }
    };

    template <typename T>
    inline void swap(free_list<T> & lhs, free_list<T> & rhs) {
        lhs.swap(rhs);
    }

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
            size_type kSizeListLen = sizeof(this->sizeList_) / sizeof(this->sizeList_[0]);
            for (size_type i = 0; i < kSizeListLen; i++) {
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
    }; // SizeClass

    struct Span;

    struct Chunk {
        void *      ptr;       // Constructed memory ptr
        Span *      span;      // Span
        size_type   size;      // Unit size

        Chunk() : ptr(nullptr), span(nullptr), size(0) {
            this->init();
        }
        Chunk(void * ptr, Span * span, size_type size) : ptr(ptr), span(span), size(size) {
            this->init();
        }

        Chunk(const Chunk & src) : ptr(nullptr), span(nullptr), size(0) {
            this->internal_copy(src);
        }

        Chunk(Chunk && src) : ptr(nullptr), span(nullptr), size(0) {
            this->internal_swap(std::forward<Chunk>(src));
        }

        Chunk & operator = (const Chunk & rhs) {
            this->copy(rhs);
            return *this;
        }

        virtual ~Chunk() {
            this->destroy();
        }

        void init() {
            //
        }

        void destroy() {
            //
        }

        void internal_copy(const Chunk & src) {
            this->ptr = src.ptr;
            this->span = src.span;
            this->size = src.size;
        }

        void internal_swap(Chunk & other) {
            std::swap(this->ptr, other.ptr);
            std::swap(this->span, other.span);
            std::swap(this->size, other.size);
        }

        void set(void * ptr, Span * span, size_type size) {
            this->ptr = ptr;
            this->span = span;
            this->size = size;
        }

        void copy(const Chunk & src) {
            if (&src != this) {
                this->internal_copy(src);
            }
        }

        void swap(Chunk & other) {
            if (&other != this) {
                this->internal_swap(other);
            }
        }

        template <typename U>
        U * valueOf(std::uint16_t offset) const {
            assert(this->ptr != nullptr);
            return reinterpret_cast<U *>(&reinterpret_cast<ChunkUnitType *>(this->ptr)[offset]);
        }

        template <typename U>
        U * valueOf(size_type offset) const {
            return this->valueOf<U>(static_cast<std::uint16_t>(offset));
        }
    }; // Chunk

    struct Span {
        void *      ptr;       // Allocated memory ptr
        size_type   start;     // The starting chunk index
        size_type   length;    // The length of span crossing

        Span() : ptr(nullptr), start(0), length(0) {
            this->init();
        }
        Span(void * ptr, size_type start, size_type length) :
            ptr(ptr), start(start), length(length) {
            this->init();
        }

        Span(const Span & src) : ptr(nullptr), start(0), length(0) {
            this->internal_copy(src);
        }

        Span(Span && src) : ptr(nullptr), start(0), length(0) {
            this->internal_swap(std::forward<Span>(src));
        }

        Span & operator = (const Span & rhs) {
            this->copy(rhs);
            return *this;
        }

        virtual ~Span() {
            this->destory();
        }

        void init() {
            //
        }

        void destory() {
            //
        }

        void internal_copy(const Chunk & src) {
            this->ptr = src.ptr;
            this->start = src.start;
            this->length = src.length;
        }

        void internal_swap(const Chunk & src) {
            std::swap(this->ptr, src.ptr);
            std::swap(this->start, src.start);
            std::swap(this->length, src.length);
        }

        void set(void * ptr, size_type start, size_type length) {
            this->ptr = ptr;
            this->start = start;
            this->length = length;
        }
    }; // Span

    class ThreadCache {
    private:
        //

    public:
        ThreadCache() {}
        ~ThreadCache() {}
    }; // ThreadCache

    class ChunkHeap {
    private:
        std::list<Span>                 span_list_;
        std::vector<Chunk>              chunk_list_;
        ThreadCache                     cache_;
#if JM_MALLOC_USE_STATISTIC_INFO
        size_type                       alloc_size_;
        size_type                       object_cnt_;
        size_type                       inuse_chunk_size_;
        size_type                       alloc_chunk_size_;
#endif
        jstd::BitSet<kChunkHighCount>   chunk_used_;

        void init() {
            this->chunk_used_.unit_size();
        }

        void internal_swap(ChunkHeap & other) {
            std::swap(this->span_list_, other.span_list_);
            std::swap(this->chunk_list_, other.chunk_list_);
            std::swap(this->cache_, other.cache_);
#if JM_MALLOC_USE_STATISTIC_INFO
            std::swap(this->alloc_size_, other.alloc_size_);
            std::swap(this->object_cnt_, other.object_cnt_);
            std::swap(this->inuse_chunk_size_, other.inuse_chunk_size_);
            std::swap(this->alloc_chunk_size_, other.alloc_chunk_size_);
#endif
            std::swap(this->chunk_used_, other.chunk_used_);
        }

    public:
#if JM_MALLOC_USE_STATISTIC_INFO
        ChunkHeap() : alloc_size_(0), object_cnt_(0), inuse_chunk_size_(0), alloc_chunk_size_(0) {
            this->init();
        }

        ChunkHeap(const ChunkHeap & src) = delete;

        ChunkHeap(ChunkHeap && src) :
            alloc_size_(0), object_cnt_(0), inuse_chunk_size_(0), alloc_chunk_size_(0) {
            this->internal_swap(std::forward<ChunkHeap>(src));
        }
#else
        ChunkHeap() {
            this->init();
        }

        ChunkHeap(const ChunkHeap & src) = delete;

        ChunkHeap(ChunkHeap && src) {
            this->internal_swap(std::forward<ChunkHeap>(src));
        }
#endif // JM_MALLOC_USE_STATISTIC_INFO

        ~ChunkHeap() {
            this->destory();
        }

        size_type chunk_size() const {
            return this->chunk_list_.size();
        }

#if JM_MALLOC_USE_STATISTIC_INFO
        size_type alloc_size() const {
            return this->alloc_size_;
        }

        size_type object_count() const {
            return this->object_cnt_;
        }

        size_type actual_inuse_size() const {
            return (this->inuse_chunk_size_ * kChunkTotalBytes);
        }

        size_type actual_alloc_size() const {
            return (this->alloc_chunk_size_ * kChunkTotalBytes);
        }
#endif
        void swap(ChunkHeap & other) {
            if (&other != this) {
                this->internal_swap(other);
            }
        }

        void destory() {
            auto iter = this->span_list_.begin();
            while (iter != this->span_list_.end()) {
                Span & span = *iter;
                this->destroySpan(&span);
                iter++;
            }
            this->span_list_.clear();
        }

        Span * findFreeSpan(size_type num_chunks) {
            return nullptr;
        }

        bool createNewSpan(Span & span, size_type num_chunks) {
            size_type first_free_chunk = 0;
            if (first_free_chunk != size_type(-1)) {
#if defined(_MSC_VER)
                void * new_ptr = ::_aligned_malloc(kChunkTotalBytes * num_chunks, kChunkAlignment);
#else
                // Note: Use posix_memalign() in BSD.
                void * new_ptr = ::memalign(kChunkAlignment, kChunkTotalBytes * num_chunks);
#endif
                if (new_ptr != nullptr) {
                    span.set(new_ptr, first_free_chunk, num_chunks);
                    return true;
                }
                else {
                    throw std::bad_alloc();
                }
            }
            return false;
        }

        bool allocateSpan(Span & span, size_type num_chunks) {
            Span * freeSpan = findFreeSpan(num_chunks);
            if (freeSpan == nullptr) {
                return this->createNewSpan(span, num_chunks);
            }
            else {
                span = freeSpan;
                return true;
            }
        }

        void destroySpan(Span * span) {
            assert(span != nullptr);
            if (span->ptr != nullptr) {
#if defined(_MSC_VER)
                ::_aligned_free(span->ptr);
#else
                ::free(span->ptr);
#endif
                span->ptr = nullptr;
            }
        }

        void releaseSpan(Span * span) {
            assert(span != nullptr);
            this->destroySpan(span);
        }

        template <typename U>
        U * realPtr(std::uint32_t handle) const {
            size_type chunk_id = (handle >> (std::uint32_t)kChunkLowShift);
            if (chunk_id < this->chunk_list_.size()) {
                const Chunk & chunk = this->chunk_list_[chunk_id];
                size_type offset = (handle & (std::uint32_t)kChunkLowMask);
                return chunk.template valueOf<U>(offset);
            }
            return nullptr;
        }

        template <typename U>
        U * realPtr(Handle handle) const {
            return this->realPtr<U>(handle.value());
        }
    }; // ChunkHeap

    struct StaticData {
        bool inited;
        SizeClass sizeClass;

        StaticData() : inited(false) {}
        ~StaticData() {}

        void init() {
            this->inited = true;
        }
    }; // StaticData

private:
    ChunkHeap   chunk_heap_;
    size_type   alloc_size_;

    void init() {
        this->Static().init();
    }

public:
    ThreadMalloc() : alloc_size_(0) {
        this->init();
    }

    ThreadMalloc(const malloc_type & src) = delete;

    ThreadMalloc(malloc_type && src) : alloc_size_(0) {
        this->internal_swap(std::forward<malloc_type>(src));
    }

    virtual ~ThreadMalloc() {
        this->destroyHeap();
    }

    void internal_swap(malloc_type & other) {
        this->chunk_heap_.swap(other.chunk_heap_);
        std::swap(this->alloc_size_, other.alloc_size_);
    }

    void swap(malloc_type & other) {
        if (&other != this) {
            this->internal_swap(other);
        }
    }

    size_type chunk_size() const {
        return this->chunk_heap_.chunk_size();
    }

#if JM_MALLOC_USE_STATISTIC_INFO
    size_type alloc_size() const {
        return this->chunk_heap_.alloc_size();
    }

    size_type object_count() const {
        return this->chunk_heap_.object_count();
    }

    size_type actual_inuse_size() const {
        return this->chunk_heap_.actual_inuse_size();
    }

    size_type actual_alloc_size() const {
        return this->chunk_heap_.actual_alloc_size();
    }
#endif

    void destroyHeap() {
        this->chunk_heap_.destory();
        this->alloc_size_ = 0;
    }

    static StaticData & Static() {
        static StaticData static_data;
        return static_data;
    }

    static malloc_type & getInstance() {
        static malloc_type malloc;
        return malloc;
    }

    Handle jm_malloc(size_type size) {
        return 0;
    }

    void jm_free(Handle handle, size_type size) {
        //
    }

    template <typename U>
    U * realPtr(std::uint32_t handle) const {
        return this->chunk_heap_.template realPtr<U>(handle);
    }

    template <typename U>
    U * realPtr(Handle handle) const {
        return this->chunk_heap_.template realPtr<U>(handle.value());
    }

    static void shutdown() {
        auto & malloc = malloc_type::getInstance();
        malloc.destroyHeap();
    }
};

#pragma pack(pop)

} // namespace jm_malloc
