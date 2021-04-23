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
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  ssize_type;

    typedef Board                               board_type;
    typedef jm_malloc::ThreadMalloc<PoolId>     malloc_type;
    typedef typename malloc_type::Handle        Handle;

    static const size_type BoardX = board_type::Y;
    static const size_type BoardY = board_type::X;
    static const size_type BoardSize = board_type::BoardSize;

    static const size_type kBitMask = (size_type(1) << Bits) - 1;

    static const size_type      kArraySizeThreshold = 16384;
    static const size_type      kMaxArraySize = size_type(1) << (Bits * BoardX - 1);
    static const std::uint16_t  kInvalidIndex = std::uint16_t(-1);
    static const int            kInvalidIndex32 = -1;

    struct NodeType {
        enum type {
            ArrayContainer,
            BitmapContainer,
            LeafArrayContainer,
            LeafBitmapContainer
        };
    };

    class Node;

    class ValueArray {
    private:
        std::vector<std::uint16_t> array_;

    public:
        ValueArray() {}
        ~ValueArray() {}

        size_type size() const {
            return this->array_.size();
        }

        void reserve(size_type capacity) {
            this->array_.reserve(capacity);
        }

        void createArray(size_type newSize) {
            this->array_.resize(newSize, kInvalidIndex);
        }

        int indexOf(std::uint16_t value) const {
            assert(value != kInvalidIndex);
            assert(this->array_.size() <= kArraySizeThreshold);
            if (this->array_.size() > 0) {
                int index = 0;
                for (auto & iter : this->array_) {
                    assert(iter != kInvalidIndex);
                    if (iter != value)
                        index++;
                    else
                        return index;                    
                }
            }
            return kInvalidIndex32;
        }

        void append(std::uint16_t value) {
            assert(this->array_.size() <= kArraySizeThreshold);
            assert(this->array_.size() <= kMaxArraySize);
            this->array_.push_back(value);
        }
    };

    class NodeArray {
    private:
        std::vector<Node *> array_;

    public:
        NodeArray() {}
        ~NodeArray() {}

        size_type size() const {
            return this->array_.size();
        }

        void reserve(size_type capacity) {
            this->array_.reserve(capacity);
        }

        void createArray(size_type newSize) {
            this->array_.resize(newSize, nullptr);
        }

        Node * valueOf(int index) const {
            assert(index != (int)kInvalidIndex);
            assert(index != kInvalidIndex32);
            assert(index >= 0 && index < (int)kInvalidIndex);
            assert(this->array_.size() <= kMaxArraySize);
            return this->array_[index];
        }

        void append(Node * node) {
            assert(node != nullptr);
            assert(this->array_.size() <= kArraySizeThreshold);
            assert(this->array_.size() <= kMaxArraySize);
            this->array_.push_back(node);
        }
    };

    class Container {
    public:
        Container() {}
        virtual ~Container() {}

        virtual void destroy() {
            //
        }

        virtual size_type begin() const {
            return 0;
        }

        virtual size_type end() const {
            return 0;
        }

        virtual void next(size_type & pos) const {
            pos++;
        }

        virtual size_type size() const {
            return 0;
        }

        virtual Node * hasChild(std::uint16_t value) const {
            return nullptr;
        }

        virtual bool hasLeafChild(std::uint16_t value) const {
            return nullptr;
        }

        virtual void append(Node * node, std::uint16_t value) {
            //
        }

        virtual Node * valueOf(size_type index) const {
            return nullptr;
        }
    };

    class ArrayContainer : public Container {
    private:
        ValueArray  valueArray_;
        NodeArray   nodeArray_;

    public:
        ArrayContainer() {}
        virtual ~ArrayContainer() {}

        void destroy() final {
            //
        }

        size_type begin() const final {
            return 0;
        }

        size_type end() const final {
            return this->valueArray_.size();
        }

        void next(size_type & pos) const final {
            pos++;
        }

        size_type size() const final {
            return this->valueArray_.size();
        }

        void reserve(size_type capacity) {
            this->valueArray_.reserve(capacity);
            this->nodeArray_.reserve(capacity);
        }

        void createArray(size_type newSize) {
            this->valueArray_.createArray(newSize);
            this->nodeArray_.createArray(newSize);
        }

        Node * hasChild(std::uint16_t value) const final {
            int index = this->valueArray_.indexOf(value);
            if (index >= 0) {
                Node * childNode = this->nodeArray_.valueOf(index);
                return childNode;
            }
            return nullptr;
        }

        bool hasLeafChild(std::uint16_t value) const final {
            return false;
        }

        Node * valueOf(size_type index) const final {
            return this->nodeArray_.valueOf((int)index);
        }

        void append(Node * node, std::uint16_t value) final {
            assert(node != nullptr);
            assert(this->valueArray_.size() <= kArraySizeThreshold);
            assert(this->valueArray_.size() <= kMaxArraySize);
            assert(this->valueArray_.size() == this->nodeArray_.size());
            this->valueArray_.append(value);
            this->nodeArray_.append(node);
        }
    };

    class LeafArrayContainer : public Container {
    private:
        ValueArray  valueArray_;

    public:
        LeafArrayContainer() {}
        virtual ~LeafArrayContainer() {}

        void destroy() final {
            //
        }

        size_type begin() const final {
            return 0;
        }

        size_type end() const final {
            return this->valueArray_.size();
        }

        void next(size_type & pos) const final {
            pos++;
        }

        size_type size() const final {
            return this->valueArray_.size();
        }

        void reserve(size_type capacity) {
            this->valueArray_.reserve(capacity);
        }

        void createArray(size_type newSize) {
            this->valueArray_.createArray(newSize);
        }

        Node * hasChild(std::uint16_t value) const final {
            return nullptr;
        }

        bool hasLeafChild(std::uint16_t value) const final {
            int index = valueArray_.indexOf(value);
            return (index >= 0);
        }

        Node * valueOf(size_type index) const final {
            return nullptr;
        }

        void append(Node * node, std::uint16_t value) final {
            assert(this->valueArray_.size() <= kArraySizeThreshold);
            assert(this->valueArray_.size() <= kMaxArraySize);
            this->valueArray_.append(value);
        }
    };

    class BitmapContainer : public Container {
    public:
        BitmapContainer() {}
        virtual ~BitmapContainer() {}

        void destroy() final {
            //
        }

        size_type begin() const final {
            return 0;
        }

        size_type end() const final {
            return kMaxArraySize;
        }

        void next(size_type & pos) const final {
            pos++;
        }

        size_type size() const final {
            return 0;
        }

        Node * hasChild(std::uint16_t value) const final {
            return nullptr;
        }

        bool hasLeafChild(std::uint16_t value) const final {
            return false;
        }

        Node * valueOf(size_type index) const final {
            return nullptr;
        }

        void append(Node * node, std::uint16_t value) final {
            //
        }
    };

    class LeafBitmapContainer : public Container {
    public:
        LeafBitmapContainer() {}
        virtual ~LeafBitmapContainer() {}

        void destroy() final {
            //
        }

        size_type begin() const final {
            return 0;
        }

        size_type end() const final {
            return kMaxArraySize;
        }

        void next(size_type & pos) const final {
            pos++;
        }

        size_type size() const final {
            return 0;
        }

        Node * hasChild(std::uint16_t value) const final {
            return nullptr;
        }

        bool hasLeafChild(std::uint16_t value) const final {
            return false;
        }

        Node * valueOf(size_type index) const final {
            return nullptr;
        }

        void append(Node * node, std::uint16_t value) final {
            //
        }
    };

    class Node {
    public:
        typedef std::size_t size_type;

        static const size_type kDefaultArraySize = 4;

    private:
        Container * container_;
        uint16_t    type_;
        uint16_t    size_;      // Cardinality

    public:
        Node() : container_(nullptr), type_(NodeType::ArrayContainer), size_(0) {}
        Node(const Node & src) = delete;
        ~Node() = default;

        Container * container() const {
            return this->container_;
        }

        size_type type() const {
            return this->type_;
        }

        size_type size() const {
            return this->size_;
        }

        void init(bool isLeaf = false) {
            assert(this->size_ == 0);
            if (!isLeaf) {
                ArrayContainer * newContainer = new ArrayContainer;
                newContainer->reserve(kDefaultArraySize);
                this->container_ = static_cast<Container *>(newContainer);
            }
            else {
                this->type_ = NodeType::LeafArrayContainer;
                LeafArrayContainer * newContainer = new LeafArrayContainer;
                newContainer->reserve(kDefaultArraySize);
                this->container_ = static_cast<Container *>(newContainer);
            }
        }

        void destroy() {
            if (this->container_) {
                this->container_->destroy();
                delete this->container_;
                this->container_ = nullptr;
            }
        }

        bool exists(size_type value) const {
            return (this->hasChild(value) != nullptr);
        }

        Node * hasChild(std::uint16_t value) const {
            if (this->type_ == NodeType::ArrayContainer ||
                this->type_ == NodeType::BitmapContainer) {
                return this->container_->hasChild(value);
            }
            else {
                return nullptr;
            }
        }

        Node * hasChild(std::size_t value) const {
            return this->hasChild(static_cast<std::uint16_t>(value));
        }

        bool hasLeafChild(std::uint16_t value) {
            if (this->type_ == NodeType::ArrayContainer ||
                this->type_ == NodeType::BitmapContainer) {
                return (this->container_->hasChild(value) != nullptr);
            }
            else if (this->type_ == NodeType::LeafArrayContainer ||
                     this->type_ == NodeType::LeafBitmapContainer) {
                return this->container_->hasLeafChild(value);
            }
            else {
                return false;
            }
        }

        bool hasLeafChild(std::size_t value) {
            return this->hasLeafChild(static_cast<std::uint16_t>(value));
        }

        Node * append(std::uint16_t value) {
            if (this->type_ == NodeType::ArrayContainer ||
                this->type_ == NodeType::BitmapContainer) {
                Node * node = new Node;
                node->init(false);
                this->container_->append(node, value);
                this->size_++;
                return node;
            }
            else {
                return nullptr;
            }
        }

        Node * append(std::size_t value) {
            return this->append(static_cast<std::uint16_t>(value));
        }

        Node * appendLeaf(std::uint16_t value) {
            if (this->type_ == NodeType::ArrayContainer ||
                this->type_ == NodeType::BitmapContainer) {
                Node * node = new Node;
                node->init(true);
                this->container_->append(node, value);
                this->size_++;
                return node;
            }
            else if (this->type_ == NodeType::LeafArrayContainer ||
                     this->type_ == NodeType::LeafBitmapContainer) {
                this->container_->append(nullptr, value);
                this->size_++;
                return nullptr;
            }
            else {
                return nullptr;
            }
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

    Node * createNewNode() {
        Node * newNode = new Node;
        newNode->init();
        return newNode;
    }

    static void shutdown() {
        //
    }
};

#pragma pack(pop)

} // namespace PuzzleGame
