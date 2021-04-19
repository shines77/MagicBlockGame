#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstdint>
#include <cstddef>

#include <vector>
#include <bitset>

#include "Value128.h"

namespace PuzzleGame {

#pragma pack(push, 1)

template <typename Key, std::size_t Bits, std::size_t ChunkSize = 65536>
class SparseTrieBitmapPool {
public:
    typedef std::size_t     size_type;
    typedef Key             key_type;

    // Node
    class Node {
    public:
        typedef std::size_t         size_type;
        typedef Key                 key_type;
        typedef Node                node_type;

        static const size_type kBitMask = (size_type(2) << Bits) - 1;

    private:
        uint16_t    bits_;
        uint16_t    size_;
        uint32_t    child_;

    public:
        Node() : bits_(0), size_(0), child_(0) {}
        ~Node() {}
    }; // End of Node

    // Chunk
    class Chunk {
    public:
        typedef std::size_t         size_type;
        typedef Key                 key_type;
        typedef Chunk               chunk_type;

        static const size_type kChunkSize = ChunkSize;

    private:
        Node * chunk_;

    public:
        Chunk() : chunk_(nullptr) {}
        ~Chunk() {}
    }; // End of Chunk

    typedef Node            node_type;
    typedef Chunk           chunk_type;

private:
    std::vector<Chunk>  chunk_list_;
    size_type           size_;

public:
    SparseTrieBitmapPool() : size_(0) {
        //
    }

    virtual ~SparseTrieBitmapPool() {
        this->destroy();
    }

    size_type size() const {
        return this->size_;
    }

    void destroy() {
        //
    }
};

#pragma pack(pop)

template <std::size_t Bits, std::size_t Length, std::size_t ChunkSize = 65536>
class SparseTrieBitmap {
public:
    typedef std::size_t                                     size_type;
    typedef Value128                                        key_type;
    typedef SparseTrieBitmapPool<key_type, Bits, ChunkSize> pool_type;
    typedef typename pool_type::chunk_type                  chunk_type;
    typedef typename pool_type::node_type                   node_type;

private:
    pool_type *     pool_;
    size_type       size_;

public:
    SparseTrieBitmap() : pool_(nullptr), size_(0) {
        //
    }

    virtual ~SparseTrieBitmap() {
        this->destroy();
    }

    size_type size() const {
        return this->size_;
    }

    void destroy() {
        if (this->pool_ != nullptr) {
            this->pool_->destroy();
            this->pool_ = nullptr;
        }
    }

    uint16_t getValue(size_type pos) {
        //
    }

    bool contains(const key_type & key) const {
        //
        return true;
    }

    void append(const key_type & key) {
        //
    }

    void remove(const key_type & key) {
        //
    }
};

} // namespace PuzzleGame
