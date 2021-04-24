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

#include "SparseBitsetUtils.h"
#include "Value128.h"

namespace MagicBlock {

template <typename Board, std::size_t Bits, std::size_t Length, std::size_t PoolId = 0>
class SparseBitset {
public:
    typedef Board                                   board_type;
    typedef SparseBitsetUtils<Board, Bits, PoolId>  utils_type;
    typedef typename utils_type::node_type          node_type;
    typedef typename utils_type::Container          Container;

    typedef typename utils_type::size_type      size_type;
    typedef typename utils_type::ssize_type     ssize_type;

    static const size_type BoardX = board_type::Y;
    static const size_type BoardY = board_type::X;
    static const size_type BoardSize = board_type::BoardSize;

    static const size_type kBitMask = utils_type::kBitMask;

    struct LayerInfo {
        size_type maxLayerSize;
        size_type childCount;
        size_type totalLayerSize;
    };

private:
    utils_type      utils_;
    size_type       size_;
    node_type *     root_;
    LayerInfo       layer_info_[BoardY];
    size_type       y_index_[BoardY];

public:
    SparseBitset() : size_(0), root_(nullptr) {
        this->init();
    }

    virtual ~SparseBitset() {
        this->destroy();
    }

    size_type size() const {
        return this->size_;
    }

    void init() {
#if 1
        size_type top = BoardY / 2 - 1, bottom = BoardY / 2;
        if ((BoardY % 2) != 0) {
            this->y_index_[0] = bottom;
            bottom++;
        }
        for (size_type yi = 0; yi < (BoardY / 2); yi++) {
            this->y_index_[yi * 2 + 2] = top--;
            this->y_index_[yi * 2 + 1] = bottom++;
        }
#else
        size_type top = 0, bottom = BoardY - 1;
        for (size_type yi = 0; yi < (BoardY / 2); yi++) {
            this->y_index_[yi * 2 + 0] = top++;
            this->y_index_[yi * 2 + 1] = bottom--;
        }
        if ((BoardY % 2) != 0) {
            this->y_index_[BoardY - 1] = top;
        }
#endif

        this->root_ = this->utils_.createNewNode();

        for (size_type i = 0; i < BoardY; i++) {
            this->layer_info_[i].maxLayerSize = 0;
            this->layer_info_[i].childCount = 0;
            this->layer_info_[i].totalLayerSize = 0;
        }
    }

    void destroy() {
        this->destroy_trie();
        this->utils_.destroy();
    }

    void shutdown() {
        this->destroy();
    }

    void destroy_trie_impl(node_type * node, size_type depth) {
        assert(node != nullptr);
        size_type layer_size = node->size();
        if (layer_size > this->layer_info_[depth].maxLayerSize) {
            this->layer_info_[depth].maxLayerSize = layer_size;
        }
        this->layer_info_[depth].childCount++;
        this->layer_info_[depth].totalLayerSize += layer_size;

        if (node->type() == utils_type::NodeType::LeafArrayContainer ||
            node->type() == utils_type::NodeType::LeafBitmapContainer) {
            return;
        }

        Container * container = node->container();
        assert(container != nullptr);
        assert(layer_size == container->size());
        for (size_type i = container->begin(); i < container->end(); container->next(i)) {
            node_type * child = container->valueOf(i);
            if (child != nullptr) {
                destroy_trie_impl(child, depth + 1);
                child->destroy();
                delete child;
            }
        }
    }

    void destroy_trie() {
        node_type * node = this->root_;
        if (node == nullptr) {
            return;
        }

        size_type layer_size = node->size();
        if (layer_size > this->layer_info_[0].maxLayerSize) {
            this->layer_info_[0].maxLayerSize = layer_size;
        }
        this->layer_info_[0].childCount++;
        this->layer_info_[0].totalLayerSize += layer_size;

        Container * container = node->container();
        assert(container != nullptr);
        assert(layer_size == container->size());
        for (size_type i = container->begin(); i < container->end(); container->next(i)) {
            node_type * child = container->valueOf(i);
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
        printf("SparseBitset<T> trie info:\n\n");
        for (size_type i = 0; i < BoardY; i++) {
            printf("[%u]: maxLayerSize = %8u, childCount = %8u, averageSize = %0.2f\n\n",
                   uint32_t(i + 1),
                   uint32_t(this->layer_info_[i].maxLayerSize),
                   uint32_t(this->layer_info_[i].childCount),
                   (double)this->layer_info_[i].totalLayerSize / this->layer_info_[i].childCount);
        }
        printf("\n");
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
        node_type * node = this->root_;
        assert(node != nullptr);
        bool is_exists;
        // Normal node & container
        size_type yi;
        for (yi = 0; yi < BoardY - 1; yi++) {
            size_type layer_value = getLayerValue(board, yi);
            assert(node->type() == utils_type::NodeType::ArrayContainer ||
                   node->type() == utils_type::NodeType::BitmapContainer);
            node_type * child = node->hasChild(layer_value);
            if (child != nullptr) {
                node = child;
                continue;
            }
            else {
                is_exists = false;
                return is_exists;
            }
        }

        // Leaf node & container
        {
            size_type layer_value = getLayerValue(board, yi);
            assert(node->type() == utils_type::NodeType::LeafArrayContainer ||
                   node->type() == utils_type::NodeType::LeafBitmapContainer);
            is_exists = node->hasLeafChild(layer_value);
        }

        return is_exists;
    }

    //
    // Root -> (ArrayContainer)0 -> (ArrayContainer)1 -> (ArrayContainer)2 -> (LeafArrayContainer)3 -> 4444
    //
    bool append(const board_type & board) {
        node_type * node = this->root_;
        assert(node != nullptr);
        bool insert_new = false;
        // Normal node & container
        size_type yi;
        for (yi = 0; yi < BoardY - 1; yi++) {
            size_type layer_value = getLayerValue(board, yi);
            if (!insert_new) {
                assert(node->type() == utils_type::NodeType::ArrayContainer ||
                       node->type() == utils_type::NodeType::BitmapContainer);
                node_type * child = node->hasChild(layer_value);
                if (child != nullptr) {
                    node = child;
                    continue;
                }
                else {
                    insert_new = true;
                }
            }
            if (yi < (BoardY - 2))
                node = node->append(layer_value);
            else
                node = node->appendLeaf(layer_value);
        }

        // Leaf node & container
        {
            size_type layer_value = getLayerValue(board, yi);
            assert(node->type() == utils_type::NodeType::LeafArrayContainer ||
                   node->type() == utils_type::NodeType::LeafBitmapContainer);
            if (!insert_new) {
                bool isExists = node->hasLeafChild(layer_value);
                if (isExists) {
                    return false;
                }
            }
            node->appendLeaf(layer_value);
        }

        return insert_new;
    }

    bool remove(const board_type & board) {
        return true;
    }
};

} // namespace PuzzleGame
