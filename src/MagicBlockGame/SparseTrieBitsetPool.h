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

#include "Value128.h"
#include "support/CT_PowerOf2.h"

namespace PuzzleGame {

#pragma pack(push, 1)

template <std::size_t Bits, std::size_t PoolId = 0>
class SparseTrieBitsetPool {
public:
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  ssize_type;

    static const size_type kBitMask = (size_type(1) << Bits) - 1;

    static const size_type kMaxArraySize = 16384;

    static const size_type kChunkAlignment = 4096;
    static const size_type kChunkPageSize = 4096;

    // The chunk size is set to 128 KB for faster memory allocation.
    static const size_type kDefaultChunkBytes = 128 * 1024;
    static const size_type kDefaultChunkUintBytes = 4;

    static const size_type kChunkTotalBytes = jstd::compile_time::round_up_pow2<kDefaultChunkBytes>::value;
    static const size_type kChunkUintBytes = jstd::compile_time::round_up_pow2<kDefaultChunkUintBytes>::value;

    static const size_type kChunkUnitSize = kChunkTotalBytes / kChunkUintBytes;
    static const size_type kChunkLowBits = jstd::compile_time::CountTrailingZeroBits<kChunkUnitSize>::value;
    static const size_type kChunkLowMask = kChunkUnitSize - 1;

    struct NodeType {
        enum type {
            ArrayContainer,
            BitmapContainer,
            LeafArrayContainer,
            LeafBitmapContainer
        };
    };

    class Node;

    class PoolHandle {
    private:
        std::uint32_t value_;

    public:
        PoolHandle() : value_(0) {}
        PoolHandle(std::uint32_t value) : value_(value) {}
        PoolHandle(const PoolHandle & src) : value_(src.value_) {}
        ~PoolHandle() = default;

        std::uint32_t value() const { return this->value_; }

        template <typename T>
        T * ptr() const {
            SparseTrieBitsetPool & pool = SparseTrieBitsetPool::getInstance();
            return pool.realPtr<T>(this->value_);
        }
    };

    class Container {
    public:
        Container() {}
        virtual ~Container() {}

        virtual Node * hasChild(std::uint16_t value) const {
            return nullptr;
        }
    };

    class LeafContainer {
    public:
        LeafContainer() {}
        virtual ~LeafContainer() {}

        virtual bool hasLeafChild(std::uint16_t value) const {
            return nullptr;
        }
    };

    class ValueArray {
    public:
        int indexOf(std::uint16_t value) const {
            return 0;
        }
    };

    class NodeArray {
    public:
        PoolHandle getChildNode(int index) const {
            return 0;
        }
    };

    class ArrayContainer : public Container {
    private:
        PoolHandle  valueArray_;
        PoolHandle  nodeArray_;
        std::uint16_t size_;

    public:
        ArrayContainer() {}
        virtual ~ArrayContainer() {}

        Node * hasChild(std::uint16_t value) const final {
            ValueArray * pValueArray = this->valueArray_.ptr<ValueArray>();
            int index = pValueArray->indexOf(value);
            if (index >= 0) {
                NodeArray * pNodeArray = this->nodeArray_.ptr<NodeArray>();
                PoolHandle childNode = pNodeArray->getChildNode(index);
                Node * pChildNode = childNode.ptr<Node>();
                return pChildNode;
            }
            return nullptr;
        }
    };

    class LeafArrayContainer : public LeafContainer {
    private:
        PoolHandle  valueArray_;
        PoolHandle  nodeArray_;
        std::uint16_t size_;

    public:
        LeafArrayContainer() {}
        virtual ~LeafArrayContainer() {}

        bool hasLeafChild(std::uint16_t value) const final {
            ValueArray * pValueArray = this->valueArray_.ptr<ValueArray>();
            int index = pValueArray->indexOf(value);
            return (index >= 0);
        }
    };

    class BitmapContainer : public Container {
    public:
        BitmapContainer() {}
        virtual ~BitmapContainer() {}

        Node * hasChild(std::uint16_t value) const final {
            return nullptr;
        }
    };

    class LeafBitmapContainer : public LeafContainer {
    public:
        LeafBitmapContainer() {}
        virtual ~LeafBitmapContainer() {}

        bool hasLeafChild(std::uint16_t value) const final {
            return false;
        }
    };

    class Node {
    public:
        typedef std::size_t size_type;

    private:
        uint16_t    type_;
        uint16_t    size_;      // Cardinality
        uint32_t    container_;

    public:
        Node() : type_(NodeType::ArrayContainer), size_(0), container_(0) {}
        Node(const Node & src) = delete;
        ~Node() = default;

        bool exists(size_type value) const {
            return (this->hasChild(value) != nullptr);
        }

        Node * hasChild(std::uint16_t value) const {
            SparseTrieBitsetPool & pool = SparseTrieBitsetPool::getInstance();
            if (this->type_ == NodeType::ArrayContainer) {
                ArrayContainer * container = pool.realPtr<ArrayContainer>(this->container_);
                return container->hasChild(value);
            }
            else if (this->type_ == NodeType::BitmapContainer) {
                BitmapContainer * container = pool.realPtr<BitmapContainer>(this->container_);
                return container->hasChild(value);
            }
            else {
                return nullptr;
            }
        }

        Node * hasChild(std::size_t value) const {
            return this->hasChild(static_cast<std::uint16_t>(value));
        }

        bool hasLeafChild(std::uint16_t value) const {
            SparseTrieBitsetPool & pool = SparseTrieBitsetPool::getInstance();
            if (this->type_ == NodeType::LeafArrayContainer) {
                LeafArrayContainer * container = pool.realPtr<LeafArrayContainer>(this->container_);
                return container->hasLeafChild(value);
            }
            else if (this->type_ == NodeType::LeafBitmapContainer) {
                LeafBitmapContainer * container = pool.realPtr<LeafBitmapContainer>(this->container_);
                return container->hasLeafChild(value);
            }
            else {
                return false;
            }
        }

        bool hasLeafChild(std::size_t value) const {
            return this->hasLeafChild(static_cast<std::uint16_t>(value));
        }

        Node * append(std::uint16_t value) {
            return nullptr;
        }

        Node * append(std::size_t value) {
            return this->append(static_cast<std::uint16_t>(value));
        }

        Node * appendLeaf(std::uint16_t value) {
            return nullptr;
        }

        Node * appendLeaf(std::size_t value) {
            return this->appendLeaf(static_cast<std::uint16_t>(value));
        }
    }; // Node

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

        template <typename T>
        T * indexOf(std::uint16_t offset) const {
            assert(this->chunk_ != nullptr);
            return reinterpret_cast<T *>(&reinterpret_cast<size_type *>(this->chunk_)[offset]);
        }
    }; // Chunk

    typedef Node            node_type;
    typedef Chunk           chunk_type;

private:
    std::vector<chunk_type *>   chunk_list_;
    size_type                   size_;

    void init() {
        chunk_type * new_chunk = new chunk_type;
        this->chunk_list_.push_back(new_chunk);
        this->size_ = 1;
    }

public:
    SparseTrieBitsetPool() : size_(0) {
        this->init();
    }

    SparseTrieBitsetPool(const SparseTrieBitsetPool & src) = delete;

    SparseTrieBitsetPool(SparseTrieBitsetPool && src) : size_(0) {
        this->internal_swap(std::forward<SparseTrieBitsetPool>(src));
    }

    virtual ~SparseTrieBitsetPool() {
        this->destroy_pool();
    }

    void internal_swap(SparseTrieBitsetPool & other) {
        std::swap(this->chunk_list_, other.chunk_list_);
        std::swap(this->size_, other.size_);
    }

    void swap(SparseTrieBitsetPool & other) {
        if (&other != this) {
            this->internal_swap(other);
        }
    }

    size_type size() const {
        return this->size_;
    }

    void destroy_pool() {
        size_type list_size = this->chunk_list_.size();
        for (size_type i = 0; i < list_size; i++) {
            chunk_type * chunk = this->chunk_list_[i];
            if (chunk != nullptr) {
                delete this->chunk_list_[i];
                this->chunk_list_[i] = nullptr;
            }
        }
        this->chunk_list_.clear();
        this->size_ = 0;
    }

    static SparseTrieBitsetPool & getInstance() {
        static std::map<size_type, SparseTrieBitsetPool> pool_map;
        if (pool_map.count(PoolId) == 0) {
            pool_map.insert(std::make_pair(PoolId, SparseTrieBitsetPool()));
        }
        return pool_map[PoolId];
    }

    PoolHandle find_ptr(void * ptr) const {
        return 0;
    }

    PoolHandle mem_alloc(size_type size) {
        //
        return 0;
    }

    void mem_free(PoolHandle handle, size_type size) {
        //
    }

    void * malloc_ptr(size_type size) {
        PoolHandle handle = this->mem_alloc(size);
        return handle.ptr<void>();
    }

    void free_ptr(void * ptr, size_type size) {
        PoolHandle handle = this->find_ptr(ptr);
        return this->mem_free(handle, size);
    }

    template <typename T>
    PoolHandle allocate() {
        return (T *)this->mem_alloc(sizeof(T));
    }

    template <typename T>
    PoolHandle allocate(size_type size) {
        return (T *)this->mem_alloc(sizeof(T) * size);
    }

    void destroy(PoolHandle handle) {
        this->mem_free(handle, sizeof(T));
    }

    void destroy(PoolHandle handle, size_type size) {
        this->mem_free(handle, sizeof(T) * size);
    }

    template <typename T>
    T * allocate_ptr() {
        return (T *)this->malloc_ptr(sizeof(T));
    }

    template <typename T>
    T * allocate_ptr(size_type size) {
        return (T *)this->malloc_ptr(sizeof(T) * size);
    }

    template <typename T>
    void destroy_ptr(T * p) {
        this->free_ptr((void *)p, sizeof(T));
    }

    template <typename T>
    void destroy_ptr(T * p, size_type size) {
        this->free_ptr((void *)p, sizeof(T) * size);
    }

    template <typename T>
    T * realPtr(std::uint32_t index) const {
        std::uint16_t chunk_id = (index >> (std::uint32_t)kChunkLowBits);
        if (chunk_id < this->chunk_list_.size()) {
            chunk_type * chunk = this->chunk_list_[chunk_id];
            std::uint16_t offset = (index & (std::uint32_t)kChunkLowMask);
            assert(chunk != nullptr);
            return chunk->indexOf<T>(offset);
        }
        return nullptr;
    }

    static void shutdown() {
        auto & pool = SparseTrieBitsetPool::getInstance();
        pool.destroy_pool();
    }
};

#pragma pack(pop)

} // namespace PuzzleGame
