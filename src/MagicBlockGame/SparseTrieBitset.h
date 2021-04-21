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

namespace PuzzleGame {

static const std::size_t s_SparseTrieBitset_board_y_index[] = {
    0, 4, 1, 3, 2
};

#pragma pack(push, 1)

template <std::size_t Bits, std::size_t PoolId = 0>
class SparseTrieBitsetPool {
public:
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  ssize_type;

    static const size_type kBitMask = (size_type(1) << Bits) - 1;

    static const size_type kMaxArraySize = 16384;

    static const size_type kChunkAlignment = 4096;

    static const size_type kChunkSize = 65536;
    static const size_type kChunkLowBits = 16;
    static const size_type kChunkLowMask = 0xFFFF;

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

    class BitmapContainer : public Container {
    public:
        BitmapContainer() {}
        virtual ~BitmapContainer() {}

        Node * hasChild(std::uint16_t value) const final {
            return nullptr;
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

        Node * append(std::uint16_t value) {
            return nullptr;
        }

        Node * append(std::size_t value) {
            return this->append(static_cast<std::uint16_t>(value));
        }
    }; // Node

    class Chunk {
    public:
        typedef std::size_t size_type;

        static const size_type kUintBytes = sizeof(size_type);

    protected:
        void *      chunk_;
        size_type   size_;

        void init() {
#if defined(_MSC_VER)
            void * new_chunk = ::_aligned_malloc(kChunkSize * kUintBytes, kChunkAlignment);
#else
            void * new_chunk = ::memalign(kChunkAlignment, kChunkSize * kUintBytes);
#endif
            if (new_chunk != nullptr) {
                this->chunk_ = new_chunk;
            }
            else {
                throw std::bad_alloc();
            }
            this->size_ = kChunkSize;
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
        std::uint16_t chunk_id = (index >> 16);
        if (chunk_id < this->chunk_list_.size()) {
            chunk_type * chunk = this->chunk_list_[chunk_id];
            std::uint16_t offset = (index & 0xFFFFUL);
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

template <typename Board, std::size_t Bits, std::size_t Length, std::size_t PoolId = 0>
class SparseTrieBitset {
public:
    typedef Board                                       board_type;
    typedef SparseTrieBitsetPool<Bits, PoolId>          pool_type;
    typedef typename pool_type::chunk_type              chunk_type;
    typedef typename pool_type::node_type               node_type;

    typedef typename pool_type::size_type               size_type;
    typedef typename pool_type::ssize_type              ssize_type;

    static const size_type BoardX = board_type::Y;
    static const size_type BoardY = board_type::X;
    static const size_type BoardSize = board_type::BoardSize;

    static const size_type kBitMask = pool_type::kBitMask;

private:
    pool_type *     pool_;
    size_type       size_;
    node_type *     root_;
    size_type       y_index_[BoardY];

public:
    SparseTrieBitset() : pool_(nullptr), size_(0), root_(nullptr) {
        this->init();
    }

    virtual ~SparseTrieBitset() {
        this->destroy();
    }

    size_type size() const {
        return this->size_;
    }

    void init() {
        size_type top = 0, bottom = BoardY - 1;
        for (size_type yi = 0; yi < (BoardY / 2); yi++) {
            this->y_index_[yi * 2 + 0] = top++;
            this->y_index_[yi * 2 + 1] = bottom--;
        }
        if ((BoardY % 2) != 0) {
            this->y_index_[BoardY - 1] = top;
        }

        auto & pool = pool_type::getInstance();
        this->root_ = pool.allocate_ptr<node_type>();
    }

    void destroy() {
        if (this->pool_ != nullptr) {
            this->pool_->destroy_pool();
            this->pool_ = nullptr;
        }
    }

    static void shutdown() {
        auto & pool = pool_type::getInstance();
        pool.destroy_pool();
    }

    bool contains(const board_type & board) const {
        //
        return true;
    }

    void append(const board_type & board) {
        node_type * node = this->root_;
        assert(node != nullptr);
        bool is_new = false;
        for (size_type yi = 0; yi < BoardY; yi++) {
            size_type layer = this->y_index_[yi];
            ssize_type cell = layer * BoardY;
            size_type layer_value = 0;
            for (ssize_type x = BoardX - 1; x >= 0; x--) {
                layer_value <<= 3;
                layer_value |= size_type(board.cells[cell + x] & kBitMask);
            }
            if (!is_new) {
                node_type * child = node->hasChild(layer_value);
                if (child != nullptr) {
                    node = child;
                }
                else {
                    is_new = true;
                    node = node->append(layer_value);
                }
            }
            else {
                node = node->append(layer_value);
            }
        }
    }

    void remove(const board_type & board) {
        //
    }
};

} // namespace PuzzleGame
