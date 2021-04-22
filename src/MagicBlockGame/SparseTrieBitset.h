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

#include "SparseTrieBitsetPool.h"
#include "Value128.h"

namespace PuzzleGame {

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

    size_type getLayerValue(const board_type & board, size_type yi) {
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
        //
        return true;
    }

    bool append(const board_type & board) {
        node_type * node = this->root_;
        assert(node != nullptr);
        bool insert_new = false;
        // Normal node
        for (size_type yi = 0; yi < BoardY - 1; yi++) {
            size_type layer_value = getLayerValue(board, yi);
            if (!insert_new) {
                node_type * child = node->hasChild(layer_value);
                if (child != nullptr) {
                    node = child;
                }
                else {
                    insert_new = true;
                    node = node->append(layer_value);
                }
            }
            else {
                node = node->append(layer_value);
            }
        }

        // Leaf node
        {
            size_type yi = BoardY - 1;
            size_type layer_value = getLayerValue(board, yi);
            insert_new = node->hasLeafChild(layer_value);
        }

        return insert_new;
    }

    bool remove(const board_type & board) {
        return true;
    }
};

} // namespace PuzzleGame
