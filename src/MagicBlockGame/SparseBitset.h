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

#define SPARSEBITSET_DISPLAY_TRIE_INFO  0

namespace MagicBlock {

template <typename Board, std::size_t Bits, std::size_t Length, std::size_t PoolId = 0>
class SparseBitset {
public:
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      ssize_type;

    typedef Board               board_type;

    static const size_type      BoardX = board_type::Y;
    static const size_type      BoardY = board_type::X;
    static const size_type      BoardSize = board_type::BoardSize;

    static const size_type      kBitMask = (size_type(1) << Bits) - 1;

    static const size_type      kDefaultArrayCapacity = 4;

    static const size_type      kArraySizeThreshold = 16384;
    static const size_type      kMaxArraySize = size_type(1) << (Bits * BoardX - 1);
    static const std::uint16_t  kInvalidIndex = std::uint16_t(-1);
    static const int            kInvalidIndex32 = -1;

#pragma pack(push, 1)

    struct LayerInfo {
        size_type maxLayerSize;
        size_type childCount;
        size_type totalLayerSize;
    };

    struct NodeType {
        enum type {
            ArrayContainer,
            BitmapContainer,
            LeafArrayContainer,
            LeafBitmapContainer
        };
    };

    class Container;

    class ValueArray {
    private:
        std::vector<std::uint16_t> array_;

    public:
        ValueArray() = default;
        ~ValueArray() = default;

        size_type size() const {
            return this->array_.size();
        }

        void reserve(size_type capacity) {
            this->array_.reserve(capacity);
        }

        void resize(size_type newSize) {
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
        std::vector<Container *> array_;

    public:
        NodeArray() = default;
        ~NodeArray() = default;

        size_type size() const {
            return this->array_.size();
        }

        void reserve(size_type capacity) {
            this->array_.reserve(capacity);
        }

        void resize(size_type newSize) {
            this->array_.resize(newSize, nullptr);
        }

        Container * valueOf(int index) const {
            assert(index != (int)kInvalidIndex);
            assert(index != kInvalidIndex32);
            assert(index >= 0 && index < (int)kInvalidIndex);
            assert(this->array_.size() <= kMaxArraySize);
            return this->array_[index];
        }

        void append(Container * container) {
            assert(container != nullptr);
            assert(this->array_.size() <= kArraySizeThreshold);
            assert(this->array_.size() <= kMaxArraySize);
            this->array_.push_back(container);
        }
    };

    class Container {
    public:
        typedef std::size_t size_type;

    protected:
        uint16_t    type_;
        uint16_t    size_;      // Cardinality
        uint16_t    capacity_;
        uint16_t    reserve_;

        void init() {
            assert(this->size_ == 0);
            this->reserve(kDefaultArrayCapacity);
            //this-capacity_ = kDefaultArrayCapacity;
        }

    public:
        Container() : type_(NodeType::ArrayContainer), size_(0), capacity_(0), reserve_(0) {
            this->init();
        }
        Container(uint16_t type) : type_(type), size_(0), capacity_(0), reserve_(0) {
            this->init();
        }

        Container(const Container & src) = delete;

        virtual ~Container() = default;

        size_type type() const {
            return this->type_;
        }

        size_type size() const {
            return this->size_;
        }

        size_type capacity() const {
            return this->capacity_;
        }

        virtual void destroy() {
            // Not implemented!
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

        virtual void reserve(size_type capacity) {
            // Not implemented!
        }

        virtual void resize(size_type newSize) {
            // Not implemented!
        }

        template <size_type type = NodeType::ArrayContainer>
        static Container * createNewContainer() {
            Container * container;
            if (type == NodeType::ArrayContainer) {
                ArrayContainer * newContainer = new ArrayContainer;
                container = static_cast<Container *>(newContainer);
            }
            else if (type == NodeType::LeafArrayContainer) {
                LeafArrayContainer * newContainer = new LeafArrayContainer;
                container = static_cast<Container *>(newContainer);
            }
            else if (type == NodeType::BitmapContainer) {
                BitmapContainer * newContainer = new BitmapContainer;
                container = static_cast<Container *>(newContainer);
            }
            else if (type == NodeType::LeafBitmapContainer) {
                LeafBitmapContainer * newContainer = new LeafBitmapContainer;
                container = static_cast<Container *>(newContainer);
            }
            else {
                container = nullptr;
                throw std::exception("createContainer(): Unknown container type");
            }
            return container;
        }

        bool exists(size_type value) const {
            return (this->hasChild(value) != nullptr);
        }

        virtual Container * hasChild(std::uint16_t value) const {
            // Not implemented!
            return nullptr;
        }

        Container * hasChild(std::size_t value) const {
            return this->hasChild(static_cast<std::uint16_t>(value));
        }

        virtual bool hasLeafChild(std::uint16_t value) const {
            // Not implemented!
            return nullptr;
        }

        bool hasLeafChild(std::size_t value) const {
            return this->hasLeafChild(static_cast<std::uint16_t>(value));
        }

        virtual void append(std::uint16_t value, Container * container) {
            // Not implemented!
        }

        virtual Container * append(std::uint16_t value) {
            // Not implemented!
            return nullptr;
        }

        Container * append(std::size_t value) {
            return this->append(static_cast<std::uint16_t>(value));
        }

        virtual Container * appendLeaf(std::uint16_t value) {
            // Not implemented!
            return nullptr;
        }

        Container * appendLeaf(std::size_t value) {
            return this->appendLeaf(static_cast<std::uint16_t>(value));
        }

        virtual Container * valueOf(int index) const {
            // Not implemented!
            return nullptr;
        }

        Container * valueOf(size_type index) const {
            return this->valueOf(static_cast<int>(index));
        }
    };

    class ArrayContainer final : public Container {
    private:
        ValueArray  valueArray_;
        NodeArray   nodeArray_;

    public:
        ArrayContainer() : Container(NodeType::ArrayContainer) {}
        ArrayContainer(const ArrayContainer & src) = delete;
        virtual ~ArrayContainer() = default;

        void destroy() final {
            // TODO:
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

        void reserve(size_type capacity) final {
            this->valueArray_.reserve(capacity);
            this->nodeArray_.reserve(capacity);
        }

        void resize(size_type newSize) final {
            this->valueArray_.resize(newSize);
            this->nodeArray_.resize(newSize);
        }

        Container * hasChild(std::uint16_t value) const final {
            int index = this->valueArray_.indexOf(value);
            if (index >= 0) {
                Container * child = this->nodeArray_.valueOf(index);
                return child;
            }
            return nullptr;
        }

        bool hasLeafChild(std::uint16_t value) const final {
            return false;
        }

        void append(std::uint16_t value, Container * container) final {
            assert(container != nullptr);
            assert(this->valueArray_.size() <= kArraySizeThreshold);
            assert(this->valueArray_.size() <= kMaxArraySize);
            assert(this->valueArray_.size() == this->nodeArray_.size());
            this->valueArray_.append(value);
            this->nodeArray_.append(container);
        }

        Container * append(std::uint16_t value) final {
            Container * container = Container::createNewContainer<NodeType::ArrayContainer>();
            this->append(value, container);
            this->size_++;
            return container;
        }

        Container * appendLeaf(std::uint16_t value) final {
            Container * container = Container::createNewContainer<NodeType::LeafArrayContainer>();
            this->append(value, container);
            this->size_++;
            return container;
        }

        Container * valueOf(int index) const final {
            return this->nodeArray_.valueOf(index);
        }
    };

    class LeafArrayContainer final : public Container {
    private:
        ValueArray  valueArray_;

    public:
        LeafArrayContainer() : Container(NodeType::LeafArrayContainer) {}
        LeafArrayContainer(const LeafArrayContainer & src) = delete;
        virtual ~LeafArrayContainer() = default;

        void destroy() final {
            // TODO:
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

        void reserve(size_type capacity) final {
            this->valueArray_.reserve(capacity);
        }

        void resize(size_type newSize) final {
            this->valueArray_.resize(newSize);
        }

        Container * hasChild(std::uint16_t value) const final {
            return nullptr;
        }

        bool hasLeafChild(std::uint16_t value) const final {
            int index = valueArray_.indexOf(value);
            return (index >= 0);
        }

        void append(std::uint16_t value, Container * container) final {
            assert(this->valueArray_.size() <= kArraySizeThreshold);
            assert(this->valueArray_.size() <= kMaxArraySize);
            this->valueArray_.append(value);
        }

        Container * append(std::uint16_t value) final {
            this->append(value, nullptr);
            this->size_++;
            return nullptr;
        }

        Container * appendLeaf(std::uint16_t value) final {
            this->append(value, nullptr);
            this->size_++;
            return nullptr;
        }

        Container * valueOf(int index) const final {
            return nullptr;
        }
    };

    class BitmapContainer final : public Container {
    public:
        BitmapContainer() : Container(NodeType::BitmapContainer) {}
        BitmapContainer(const BitmapContainer & src) = delete;
        virtual ~BitmapContainer() = default;

        void destroy() final {
            // TODO:
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

        void reserve(size_type capacity) final {
            // TODO:
        }

        void resize(size_type newSize) final {
            // TODO:
        }

        Container * hasChild(std::uint16_t value) const final {
            // TODO:
            return nullptr;
        }

        bool hasLeafChild(std::uint16_t value) const final {
            return false;
        }

        void append(std::uint16_t value, Container * container) final {
            // TODO:
        }

        Container * append(std::uint16_t value) final {
            Container * container = Container::createNewContainer<NodeType::ArrayContainer>();
            this->append(value, container);
            this->size_++;
            return container;
        }

        Container * appendLeaf(std::uint16_t value) final {
            Container * container = Container::createNewContainer<NodeType::LeafArrayContainer>();
            this->append(value, container);
            this->size_++;
            return container;
        }

        Container * valueOf(int index) const final {
            // TODO:
            return nullptr;
        }
    };

    class LeafBitmapContainer final : public Container {
    public:
        LeafBitmapContainer() : Container(NodeType::LeafBitmapContainer) {}
        LeafBitmapContainer(const LeafBitmapContainer & src) = delete;
        virtual ~LeafBitmapContainer() = default;

        void destroy() final {
            // TODO:
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

        void reserve(size_type capacity) final {
            // TODO:
        }

        void resize(size_type newSize) final {
            // TODO:
        }

        Container * hasChild(std::uint16_t value) const final {
            return nullptr;
        }

        bool hasLeafChild(std::uint16_t value) const final {
            // TODO:
            return false;
        }

        void append(std::uint16_t value, Container * container) final {
            // TODO:
        }

        Container * append(std::uint16_t value) final {
            this->append(value, nullptr);
            this->size_++;
            return nullptr;
        }

        Container * appendLeaf(std::uint16_t value) final {
            this->append(value, nullptr);
            this->size_++;
            return nullptr;
        }

        Container * valueOf(int index) const final {
            // TODO:
            return nullptr;
        }
    };

#pragma pack(pop)

private:
    Container *     root_;
    size_type       size_;
    LayerInfo       layer_info_[BoardY];
    size_type       y_index_[BoardY];

    void init() {
#if 1
        size_type top = 0, bottom = BoardY - 1;
        for (size_type yi = 0; yi < (BoardY / 2); yi++) {
            this->y_index_[yi * 2 + 0] = top++;
            this->y_index_[yi * 2 + 1] = bottom--;
        }
        if ((BoardY % 2) != 0) {
            this->y_index_[BoardY - 1] = top;
        }
#else
        size_type top = BoardY / 2 - 1, bottom = BoardY / 2;
        if ((BoardY % 2) != 0) {
            this->y_index_[0] = bottom;
            bottom++;
        }
        for (size_type yi = 0; yi < (BoardY / 2); yi++) {
            this->y_index_[yi * 2 + 1] = top--;
            this->y_index_[yi * 2 + 2] = bottom++;
        }
#endif
        this->root_ = Container::createNewContainer<NodeType::ArrayContainer>();

        for (size_type i = 0; i < BoardY; i++) {
            this->layer_info_[i].maxLayerSize = 0;
            this->layer_info_[i].childCount = 0;
            this->layer_info_[i].totalLayerSize = 0;
        }
    }

public:
    SparseBitset() : root_(nullptr), size_(0) {
        this->init();
    }

    virtual ~SparseBitset() {
        this->destroy();
    }

    size_type size() const {
        return this->size_;
    }

    void destroy() {
        this->destroy_trie();
    }

    void shutdown() {
        this->destroy();
    }

    void destroy_trie_impl(Container * container, size_type depth) {
        assert(container != nullptr);
#if SPARSEBITSET_DISPLAY_TRIE_INFO
        size_type layer_size = container->size();
        if (layer_size > this->layer_info_[depth].maxLayerSize) {
            this->layer_info_[depth].maxLayerSize = layer_size;
        }
        this->layer_info_[depth].childCount++;
        this->layer_info_[depth].totalLayerSize += layer_size;

        if (container->type() == NodeType::LeafArrayContainer ||
            container->type() == NodeType::LeafBitmapContainer) {
            return;
        }
#endif
        for (size_type i = container->begin(); i < container->end(); container->next(i)) {
            Container * child = container->valueOf(i);
            if (child != nullptr) {
#if (SPARSEBITSET_DISPLAY_TRIE_INFO == 0)
                if (child->type() != NodeType::LeafArrayContainer &&
                    child->type() != NodeType::LeafBitmapContainer) {
                    destroy_trie_impl(child, depth + 1);
                }
#endif                
                child->destroy();
                delete child;
            }
        }
    }

    void destroy_trie() {
        Container * container = this->root_;
        if (container == nullptr) {
            return;
        }

#if SPARSEBITSET_DISPLAY_TRIE_INFO
        size_type layer_size = container->size();
        if (layer_size > this->layer_info_[0].maxLayerSize) {
            this->layer_info_[0].maxLayerSize = layer_size;
        }
        this->layer_info_[0].childCount++;
        this->layer_info_[0].totalLayerSize += layer_size;
#endif
        for (size_type i = container->begin(); i < container->end(); container->next(i)) {
            Container * child = container->valueOf(i);
            if (child != nullptr) {
                destroy_trie_impl(child, 1);
                child->destroy();
                delete child;
            }
        }

        this->root_->destroy();
        delete this->root_;
        this->root_ = nullptr;
    }

    void display_trie_info() {
#if SPARSEBITSET_DISPLAY_TRIE_INFO
        printf("SparseBitset<T> trie info:\n\n");
        for (size_type i = 0; i < BoardY; i++) {
            printf("[%u]: maxLayerSize = %8u, childCount = %8u, averageSize = %0.2f\n\n",
                   uint32_t(i + 1),
                   uint32_t(this->layer_info_[i].maxLayerSize),
                   uint32_t(this->layer_info_[i].childCount),
                   (double)this->layer_info_[i].totalLayerSize / this->layer_info_[i].childCount);
        }
        printf("\n");
#endif
    }

    size_type getLayerValue(const board_type & board, size_type yi) const {
        size_type layer = this->y_index_[yi];
        ssize_type cell = layer * BoardY;
        size_type layer_value = 0;
        for (ssize_type x = BoardX - 1; x >= 0; x--) {
            layer_value <<= 3;
            layer_value |= size_type(board.cells[cell + x] & kBitMask);
        }
        return layer_value;
    }

    bool contains(const board_type & board) const {
        Container * container = this->root_;
        assert(container != nullptr);
        bool is_exists;
        // Normal container
        size_type yi;
        for (yi = 0; yi < BoardY - 1; yi++) {
            size_type layer_value = getLayerValue(board, yi);
            assert(container->type() == NodeType::ArrayContainer ||
                   container->type() == NodeType::BitmapContainer);
            Container * child = container->hasChild(layer_value);
            if (child != nullptr) {
                container = child;
                continue;
            }
            else {
                is_exists = false;
                return is_exists;
            }
        }

        // Leaf container
        {
            size_type layer_value = getLayerValue(board, yi);
            assert(container->type() == NodeType::LeafArrayContainer ||
                   container->type() == NodeType::LeafBitmapContainer);
            is_exists = container->hasLeafChild(layer_value);
        }

        return is_exists;
    }

    //
    // Root -> (ArrayContainer)0 -> (ArrayContainer)1 -> (ArrayContainer)2 -> (LeafArrayContainer)3 -> 4444
    //
    bool append(const board_type & board) {
        Container * container = this->root_;
        assert(container != nullptr);
        bool insert_new = false;
        // Normal container
        size_type yi;
        for (yi = 0; yi < BoardY - 1; yi++) {
            size_type layer_value = getLayerValue(board, yi);
            if (!insert_new) {
                assert(container->type() == NodeType::ArrayContainer ||
                       container->type() == NodeType::BitmapContainer);
                Container * child = container->hasChild(layer_value);
                if (child != nullptr) {
                    container = child;
                    continue;
                }
                else {
                    insert_new = true;
                }
            }
            if (yi < (BoardY - 2))
                container = container->append(layer_value);
            else
                container = container->appendLeaf(layer_value);
        }

        // Leaf container
        {
            size_type layer_value = getLayerValue(board, yi);
            assert(container->type() == NodeType::LeafArrayContainer ||
                   container->type() == NodeType::LeafBitmapContainer);
            if (!insert_new) {
                bool isExists = container->hasLeafChild(layer_value);
                if (isExists) {
                    return false;
                }
            }
            container->appendLeaf(layer_value);
        }

        return insert_new;
    }

    bool remove(const board_type & board) {
        return true;
    }
};

} // namespace PuzzleGame
