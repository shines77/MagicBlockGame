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
#include "jm_malloc.h"
#include "support/CT_PowerOf2.h"

using namespace jm_malloc;

namespace MagicBlock {

#pragma pack(push, 1)

template <std::size_t Bits, std::size_t PoolId = 0>
class SparseBitsetPool {
public:
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  ssize_type;

    typedef jm_malloc::ThreadMalloc<PoolId>     malloc_type;
    typedef typename malloc_type::Handle        Handle;

    static const size_type kBitMask = (size_type(1) << Bits) - 1;

    static const size_type kMaxArraySize = 16384;

    struct NodeType {
        enum type {
            ArrayContainer,
            BitmapContainer,
            LeafArrayContainer,
            LeafBitmapContainer
        };
    };

    class Node;

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
        Handle getChildNode(int index) const {
            return 0;
        }
    };

    class ArrayContainer : public Container {
    private:
        Handle  valueArray_;
        Handle  nodeArray_;
        std::uint16_t size_;

    public:
        ArrayContainer() {}
        virtual ~ArrayContainer() {}

        Node * hasChild(std::uint16_t value) const final {
            ValueArray * pValueArray = this->valueArray_.ptr<ValueArray>();
            int index = pValueArray->indexOf(value);
            if (index >= 0) {
                NodeArray * pNodeArray = this->nodeArray_.ptr<NodeArray>();
                Handle childNode = pNodeArray->getChildNode(index);
                Node * pChildNode = childNode.ptr<Node>();
                return pChildNode;
            }
            return nullptr;
        }
    };

    class LeafArrayContainer : public LeafContainer {
    private:
        Handle  valueArray_;
        Handle  nodeArray_;
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
            auto & malloc = malloc_type::getInstance();
            if (this->type_ == NodeType::ArrayContainer) {
                ArrayContainer * container = malloc.realPtr<ArrayContainer>(this->container_);
                return container->hasChild(value);
            }
            else if (this->type_ == NodeType::BitmapContainer) {
                BitmapContainer * container = malloc.realPtr<BitmapContainer>(this->container_);
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
            auto & malloc = malloc_type::getInstance();
            if (this->type_ == NodeType::LeafArrayContainer) {
                LeafArrayContainer * container = malloc.realPtr<LeafArrayContainer>(this->container_);
                return container->hasLeafChild(value);
            }
            else if (this->type_ == NodeType::LeafBitmapContainer) {
                LeafBitmapContainer * container = malloc.realPtr<LeafBitmapContainer>(this->container_);
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

    typedef Node    node_type;

private:
    size_type   alloc_size_;

    void init() {
    }

    void internal_swap(SparseBitsetPool & other) {
        std::swap(this->alloc_size_, other.alloc_size_);
    }

public:
    SparseBitsetPool() : alloc_size_(0) {
        this->init();
    }

    SparseBitsetPool(const SparseBitsetPool & src) = delete;

    SparseBitsetPool(SparseBitsetPool && src) : alloc_size_(0) {
        this->internal_swap(std::forward<SparseBitsetPool>(src));
    }

    virtual ~SparseBitsetPool() {
        this->destroy();
    }

    void swap(SparseBitsetPool & other) {
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

    Handle find_ptr(void * ptr) const {
        return 0;
    }

    Handle mem_alloc(size_type size) {
        auto & malloc = malloc_type::getInstance();
        return malloc.jm_malloc(size);
    }

    void mem_free(Handle handle, size_type size) {
        auto & malloc = malloc_type::getInstance();
        return malloc.jm_free(handle, size);
    }

    void * malloc_ptr(size_type size) {
        Handle handle = this->mem_alloc(size);
        return handle.ptr<void>();
    }

    void free_ptr(void * ptr, size_type size) {
        Handle handle = this->find_ptr(ptr);
        return this->mem_free(handle, size);
    }

    template <typename T>
    Handle allocate() {
        return (T *)this->mem_alloc(sizeof(T));
    }

    template <typename T>
    Handle allocate(size_type size) {
        return (T *)this->mem_alloc(sizeof(T) * size);
    }

    void destroy(Handle handle) {
        this->mem_free(handle, sizeof(T));
    }

    void destroy(Handle handle, size_type size) {
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

    static void shutdown() {
        auto & malloc = malloc_type::getInstance();
        malloc.shutdown();
    }
};

#pragma pack(pop)

} // namespace PuzzleGame
