#pragma once

#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <malloc.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>

#include <cstring>
#include <memory>
#include <vector>
#include <map>
#include <bitset>
#include <algorithm>        // For std::swap(), until C++11
#include <utility>          // For std::swap(), since C++11
#include <exception>
#include <stdexcept>

#include "MagicBlock/AI/Color.h"
#include "MagicBlock/AI/Board.h"
#include "MagicBlock/AI/Value128.h"
#include "MagicBlock/AI/Algorithm.h"

#define SPARSEBITSET_USE_INDEX_SORT     1
#define SPARSEBITSET_USE_TRIE_INFO      0

namespace MagicBlock {
namespace AI {

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
    static const size_type      kMaxArraySize = size_type(1) << (Bits * BoardX);
    static const std::uint16_t  kInvalidIndex = std::uint16_t(-1);
    static const int            kInvalidIndex32 = -1;

    static const size_type      kArraySizeSortThersold = 64;

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

    class IndexVector {
    private:
        std::vector<std::uint16_t> array_;

    public:
        IndexVector() noexcept {}
        ~IndexVector() {}

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

    class ValueVector {
    private:
        std::vector<Container *> array_;

    public:
        ValueVector() noexcept {}
        ~ValueVector() {}

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

    class IndexArray {
    public:
        IndexArray() noexcept {}
        ~IndexArray() {}

        size_type size() const {
            return 0;
        }

        void reserve(size_type capacity) {
        }

        void resize(size_type newSize) {
        }

        void append(std::uintptr_t * ptr, std::uint16_t size, std::uint16_t value) {
            assert(size <= kArraySizeThreshold);
            assert(size <= kMaxArraySize);
            std::uint16_t * target = (std::uint16_t *)ptr + size;
            assert(target != nullptr);
            *target = value;
        }

        int indexOf(std::uintptr_t * ptr, std::uint16_t size, std::uint16_t value) const {
            assert(size <= kArraySizeThreshold);
            assert(size <= kMaxArraySize);
            std::uint16_t * indexFirst = (std::uint16_t *)ptr;
            std::uint16_t * indexLast  = (std::uint16_t *)ptr + size;
            for (std::uint16_t * indexs = indexFirst; indexs < indexLast; indexs++) {
                assert(*indexs != kInvalidIndex);
                if (*indexs != value)
                    continue;
                else
                    return int(indexs - indexFirst);
            }
            return kInvalidIndex32;
        }

        int indexOf(std::uintptr_t * ptr, std::uint16_t size, std::uint16_t sorted, std::uint16_t value) const {
            assert(size <= kArraySizeThreshold);
            assert(size <= kMaxArraySize);
#if SPARSEBITSET_USE_INDEX_SORT
            if (sorted > 0) {
                int index = Algorithm::binary_search((std::uint16_t *)ptr, 0, sorted, value);
                if (index != kInvalidIndex32)
                    return index;
            }
#endif
            assert(sorted <= size);
#if _USE_AVX2_
            if (sorted < size)
                return (int)(Algorithm::find_uint16_avx2((std::uint16_t *)ptr, sorted, size, value));
            else
                return kInvalidIndex32;
#elif _USE_SSE2_
            if (sorted < size)
                return (int)(Algorithm::find_uint16_sse2((std::uint16_t *)ptr, sorted, size, value));
            else
                return kInvalidIndex32;
#else
            std::uint16_t * indexFirst = (std::uint16_t *)ptr + sorted;
            std::uint16_t * indexLast  = (std::uint16_t *)ptr + size;
            for (std::uint16_t * indexs = indexFirst; indexs < indexLast; indexs++) {
                assert(*indexs != kInvalidIndex);
                if (*indexs != value)
                    continue;
                else
                    return int(indexs - (std::uint16_t *)ptr);
            }
            return kInvalidIndex32;
#endif
        }

        std::uint16_t valueOf(std::uintptr_t * ptr, std::uint16_t index) const {
            assert(index != kInvalidIndex);
            assert(index >= 0 && index < (std::uint16_t)kMaxArraySize);
            std::uint16_t * value = (std::uint16_t *)ptr + index;
            return *value;
        }
    };

    class ValueArray {
    public:
        ValueArray() noexcept {}
        ~ValueArray() {}

        size_type size() const {
            return 0;
        }

        void reserve(size_type capacity) {
        }

        void resize(size_type newSize) {
        }

        void append(std::uintptr_t * ptr, std::uint16_t size, Container * container) {
            assert(container != nullptr);
            assert(size <= std::uint16_t(kMaxArraySize));
            Container ** value = (Container **)ptr + size;
            assert(value != nullptr);
            *value = container;
        }

        void append(std::uintptr_t * ptr, std::uint16_t size, std::uint16_t capacity, Container * container) {
            assert(container != nullptr);
            assert(size <= std::uint16_t(kMaxArraySize));
            assert(size <= capacity);
            std::uint16_t * indexEnd = (std::uint16_t *)ptr + capacity;
            Container ** value = (Container **)indexEnd + size;
            assert(value != nullptr);
            *value = container;
        }

        Container * valueOf(std::uintptr_t * ptr, int index) const {
            assert(index != kInvalidIndex32);
            assert(index >= 0 && index < (int)kMaxArraySize);
            Container ** value = (Container **)ptr + index;
            return *value;
        }

        Container * valueOf(std::uintptr_t * ptr, std::uint16_t capacity, int index) const {
            assert(index != kInvalidIndex32);
            assert(index >= 0 && index < (int)capacity);
            assert(capacity <= std::uint16_t(kMaxArraySize));
            std::uint16_t * indexEnd = (std::uint16_t *)ptr + capacity;
            Container ** value = (Container **)indexEnd + index;
            return *value;
        }
    };

    class Container {
    public:
        typedef std::size_t size_type;

    protected:
        std::uint16_t    type_;
        std::uint16_t    size_;      // Cardinality
        std::uint16_t    capacity_;
        std::uint16_t    sorted_;
        std::uintptr_t * ptr_;

        void init() {
            assert(this->size_ == 0);
            this->reserve(kDefaultArrayCapacity);
        }

    public:
        Container() noexcept : type_(NodeType::ArrayContainer), size_(0), capacity_(0), sorted_(0), ptr_(nullptr) {
            this->init();
        }
        Container(std::uint16_t type) noexcept : type_(type), size_(0), capacity_(0), sorted_(0), ptr_(nullptr) {
            this->init();
        }
        Container(size_type type, size_type size, size_type capacity, std::uintptr_t * ptr) noexcept
            : type_(static_cast<std::uint16_t>(type)),
              size_(static_cast<std::uint16_t>(size)),
              capacity_(static_cast<std::uint16_t>(capacity)), sorted_(0),
              ptr_(ptr) {
            this->init();
        }

        Container(const Container & src) = delete;

        virtual ~Container() {
            this->destroy();
        }

        size_type type() const {
            return this->type_;
        }

        size_type size() const {
            return this->size_;
        }

        size_type capacity() const {
            return this->capacity_;
        }

        size_type sorted() const {
            return this->sorted_;
        }

        bool isLeaf() const  {
            return (this->type() == NodeType::LeafArrayContainer ||
                    this->type() == NodeType::LeafBitmapContainer);
        }

        bool isValidType() const  {
            return (this->type() == NodeType::ArrayContainer ||
                    this->type() == NodeType::BitmapContainer ||
                    this->type() == NodeType::LeafArrayContainer ||
                    this->type() == NodeType::LeafBitmapContainer);
        }

        virtual void destroy() {
            if (this->ptr_ != nullptr) {
                std::free(this->ptr_);
                this->ptr_ = nullptr;
            }
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

        void allocate(size_type size, size_type capacity) {
            assert(this->ptr_ == nullptr);
            assert(capacity != this->capacity());
            this->ptr_ = (uintptr_t *)std::malloc(size);
            this->capacity_ = std::uint16_t(capacity);
        }

        void reallocate_orgi(size_type newSize, size_type newCapacity) {
            if (newCapacity > this->capacity()) {
                if (this->ptr_ != nullptr) {
                    uintptr_t * new_ptr = (uintptr_t *)std::malloc(newSize);
                    if (new_ptr != nullptr) {
                        std::memcpy(new_ptr, this->ptr_, this->capacity());
                        std::free(this->ptr_);
                        this->ptr_ = new_ptr;
                        this->capacity_ = std::uint16_t(newCapacity);
                    }
                }
                else {
                    this->allocate(newSize, newCapacity);
                }
            }
        }

        bool isExists(size_type value) const {
            Container * child;
            return this->hasChild(value, child);
        }

        virtual Container * getChild(std::uint16_t value) const {
            // Not implemented!
            return nullptr;
        }

        Container * getChild(std::size_t value) const {
            return this->getChild(static_cast<std::uint16_t>(value));
        }

        virtual bool hasChild(std::uint16_t value, Container *& child) const {
            // Not implemented!
            return false;
        }

        bool hasChild(std::size_t value, Container *& child) const {
            return this->hasChild(static_cast<std::uint16_t>(value), child);
        }

        virtual bool hasLeaf(std::uint16_t value) const {
            // Not implemented!
            return false;
        }

        bool hasLeaf(std::size_t value) const {
            return this->hasLeaf(static_cast<std::uint16_t>(value));
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

        virtual int indexOf(std::uint16_t index) const {
            // Not implemented!
            return kInvalidIndex;
        }

        int indexOf(size_type index) const {
            return this->indexOf(static_cast<std::uint16_t>(index));
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
        IndexArray  indexArray_;
        ValueArray  valueArray_;

        void reallocate(size_type newSize, size_type newCapacity) {
            assert(newCapacity > this->capacity());
            assert(this->ptr_ != nullptr);
            if (true) {
                std::uintptr_t * new_ptr = (std::uintptr_t *)std::malloc(newSize);
                if (new_ptr != nullptr) {
                    //assert(this->ptr_ != nullptr);
                    std::uint16_t * indexEnd = (std::uint16_t *)this->ptr_ + this->capacity();
                    std::uint16_t * newIndexEnd = (std::uint16_t *)new_ptr + newCapacity;
                    Container ** valueFirst = (Container **)indexEnd;
                    Container ** newValueFirst = (Container **)newIndexEnd;
#if SPARSEBITSET_USE_INDEX_SORT
                    if (this->capacity() <= kArraySizeSortThersold) {
                        std::memcpy(new_ptr, this->ptr_, sizeof(std::uint16_t) * this->capacity());
                        std::memcpy(newValueFirst, valueFirst, sizeof(Container *) * this->capacity());
                    }
                    else {
                        if (this->sorted() != 0) {
                            // Quick sort (second half)
                            Algorithm::quick_sort((std::uint16_t *)this->ptr_, (std::uintptr_t **)valueFirst,
                                                  this->capacity() / 2, this->capacity() - 1);
                            // (Half) Merge sort
                            Algorithm::merge_sort((std::uint16_t *)this->ptr_, (std::uintptr_t **)valueFirst,
                                                  (std::uint16_t *)new_ptr, (std::uintptr_t **)newValueFirst,
                                                  0, this->capacity());
                            this->sorted_ = this->capacity_;
                        }
                        else {
                            // Quick sort
                            Algorithm::quick_sort((std::uint16_t *)this->ptr_, (std::uintptr_t **)valueFirst,
                                                  0, this->capacity() - 1);
                            // Copy sorted array to new buffer
                            std::memcpy(new_ptr, this->ptr_, sizeof(std::uint16_t) * this->capacity());
                            std::memcpy(newValueFirst, valueFirst, sizeof(Container *) * this->capacity());
                            this->sorted_ = this->capacity_;
                        }
                    }
#else
                    std::memcpy(new_ptr, this->ptr_, sizeof(std::uint16_t) * this->capacity());
                    std::memcpy(newValueFirst, valueFirst, sizeof(Container *) * this->capacity());
#endif
                    std::free(this->ptr_);
                    this->ptr_ = new_ptr;
                    this->capacity_ = std::uint16_t(newCapacity);
                }
            }
            else {
                this->allocate(newSize, newCapacity);
            }
        }

    public:
        ArrayContainer() noexcept : Container(NodeType::ArrayContainer) {
            this->init();
        }
        ArrayContainer(const ArrayContainer & src) = delete;

        virtual ~ArrayContainer() {
            this->destroy();
        }

        void destroy() final {
            // Do nothing !!
        }

        size_type begin() const final {
            return 0;
        }

        size_type end() const final {
            return this->size();
        }

        void next(size_type & pos) const final {
            pos++;
        }

        void reserve(size_type capacity) final {
            assert(capacity > this->capacity());
            size_type allocSize = (sizeof(std::uint16_t) + sizeof(Container *)) * capacity;
            this->allocate(allocSize, capacity);
        }

        void resize(size_type newCapacity) final {
            assert (newCapacity > this->capacity());
            size_type allocSize = (sizeof(std::uint16_t) + sizeof(Container *)) * newCapacity;
            this->reallocate(allocSize, newCapacity);
        }

        Container * getChild(std::uint16_t value) const final {
            int index = this->indexArray_.indexOf(this->ptr_, this->size_, this->sorted_, value);
            assert(index >= kInvalidIndex32);
            if (index != kInvalidIndex32) {
                Container * child = this->valueArray_.valueOf(this->ptr_, this->capacity_, index);
                return child;
            }
            return nullptr;
        }

        bool hasChild(std::uint16_t value, Container *& child) const final {
            int index = this->indexArray_.indexOf(this->ptr_, this->size_, this->sorted_, value);
            assert(index >= kInvalidIndex32);
            if (index != kInvalidIndex32) {
                Container * nextChild = this->valueArray_.valueOf(this->ptr_, this->capacity_, index);
                assert(nextChild != nullptr);
                child = nextChild;
                return true;
            }
            return false;
        }

        bool hasLeaf(std::uint16_t value) const final {
            return false;
        }

        void append(std::uint16_t value, Container * container) final {
            assert(container != nullptr);
            assert(this->size() <= kArraySizeThreshold);
            assert(this->size() <= kMaxArraySize);
            if (this->size() >= this->capacity()) {
                this->resize(this->capacity() * 2);
            }
            this->indexArray_.append(this->ptr_, this->size_, value);
            this->valueArray_.append(this->ptr_, this->size_, this->capacity_, container);
        }

        Container * append(std::uint16_t value) final {
            Container * container = new ArrayContainer();
            this->append(value, container);
            this->size_++;
            return container;
        }

        Container * appendLeaf(std::uint16_t value) final {
            Container * container = new LeafArrayContainer();
            this->append(value, container);
            this->size_++;
            return container;
        }

        int indexOf(std::uint16_t index) const final {
            assert(index < this->size_);
            return this->indexArray_.valueOf(this->ptr_, index);
        }

        Container * valueOf(int index) const final {
            assert(index < (int)this->size_);
            return this->valueArray_.valueOf(this->ptr_, this->capacity_, index);
        }
    };

    class LeafArrayContainer final : public Container {
    private:
        IndexArray  indexArray_;

        void reallocate(size_type newSize, size_type newCapacity) {
            assert(newCapacity > this->capacity());
            assert(this->ptr_ != nullptr);
            if (true) {
                uintptr_t * new_ptr = (uintptr_t *)std::malloc(newSize);
                if (new_ptr != nullptr) {
                    //assert(this->ptr_ != nullptr);
#if SPARSEBITSET_USE_INDEX_SORT
                    if (this->capacity() <= kArraySizeSortThersold) {
                        std::memcpy(new_ptr, this->ptr_, sizeof(std::uint16_t) * this->capacity());
                    }
                    else {
                        if (this->sorted() != 0) {
                            // Quick sort (second half)
                            Algorithm::quick_sort((std::uint16_t *)this->ptr_,
                                                  this->capacity() / 2, this->capacity() - 1);
                            // (Half) Merge sort
                            Algorithm::merge_sort((std::uint16_t *)this->ptr_, (std::uint16_t *)new_ptr,
                                                  0, this->capacity());
                            this->sorted_ = this->capacity_;
                        }
                        else {
                            // Quick sort
                            Algorithm::quick_sort((std::uint16_t *)this->ptr_, 0, this->capacity());
                            // Copy sorted array to new buffer
                            std::memcpy(new_ptr, this->ptr_, sizeof(std::uint16_t) * this->capacity());
                            this->sorted_ = this->capacity_;
                        }
                    }
#else
                    std::memcpy(new_ptr, this->ptr_, sizeof(std::uint16_t) * this->capacity());
#endif
                    std::free(this->ptr_);
                    this->ptr_ = new_ptr;
                    this->capacity_ = std::uint16_t(newCapacity);
                }
            }
            else {
                this->allocate(newSize, newCapacity);
            }
        }

    public:
        LeafArrayContainer() noexcept : Container(NodeType::LeafArrayContainer) {
            this->init();
        }
        LeafArrayContainer(const LeafArrayContainer & src) = delete;

        virtual ~LeafArrayContainer() {
            this->destroy();
        }

        void destroy() final {
            // Do nothing !!
        }

        size_type begin() const final {
            return 0;
        }

        size_type end() const final {
            return this->size();
        }

        void next(size_type & pos) const final {
            pos++;
        }

        void reserve(size_type capacity) final {
            assert(capacity > this->capacity());
            size_type allocSize = sizeof(std::uint16_t) * capacity;
            this->allocate(allocSize, capacity);
        }

        void resize(size_type newCapacity) final {
            assert (newCapacity > this->capacity());
            size_type allocSize = sizeof(std::uint16_t) * newCapacity;
            this->reallocate(allocSize, newCapacity);
        }

        Container * getChild(std::uint16_t value) const final {
            return nullptr;
        }

        bool hasChild(std::uint16_t value, Container *& child) const final {
            child = nullptr;
            return this->hasLeaf(value);
        }

        bool hasLeaf(std::uint16_t value) const final {
            int index = indexArray_.indexOf(this->ptr_, this->size_, this->sorted_, value);
            assert(index >= kInvalidIndex32);
            return (index != kInvalidIndex32);
        }

        void append(std::uint16_t value, Container * container) final {
            assert(this->size() <= kArraySizeThreshold);
            assert(this->size() <= kMaxArraySize);
            if (this->size() >= this->capacity()) {
                this->resize(this->capacity() * 2);
            }
            this->indexArray_.append(this->ptr_, this->size_, value);
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

        int indexOf(std::uint16_t index) const final {
            assert(index < this->size_);
            return this->indexArray_.valueOf(this->ptr_, index);
        }

        Container * valueOf(int index) const final {
            assert(index < (int)this->size_);
            return nullptr;
        }
    };

    class BitmapContainer final : public Container {
    private:
        std::bitset<kMaxArraySize>  bitset_;
        ValueArray                  valueArray_;

        void init() {
            size_type allocSize = (sizeof(Container *)) * kMaxArraySize;
            this->allocate(allocSize, kMaxArraySize);
        }

    public:
        BitmapContainer() noexcept : Container(NodeType::BitmapContainer, 0, 0, nullptr) {
            this->init();
        }
        BitmapContainer(const BitmapContainer & src) = delete;

        virtual ~BitmapContainer() {
            this->destroy();
        }

        void destroy() final {
            // Do nothing !!
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
            // Do nothing !!
        }

        void resize(size_type newSize) final {
            // Do nothing !!
        }

        Container * getChild(std::uint16_t value) const final {
            bool exists = this->bitset_.test(value);
            if (exists) {
                Container * child = this->valueArray_.valueOf(this->ptr_, value);
                return child;
            }
            return nullptr;
        }

        bool hasChild(std::uint16_t value, Container *& child) const final {
            bool exists = this->bitset_.test(value);
            if (exists) {
                Container * nextChild = this->valueArray_.valueOf(this->ptr_, value);
                assert(nextChild != nullptr);
                child = nextChild;
                return true;
            }
            return false;
        }

        bool hasLeaf(std::uint16_t value) const final {
            return false;
        }

        void append(std::uint16_t value, Container * container) final {
            this->bitset_.set(value);
            this->valueArray_.append(this->ptr_, value, container);
        }

        Container * append(std::uint16_t value) final {
            Container * container = new ArrayContainer();
            this->append(value, container);
            this->size_++;
            return container;
        }

        Container * appendLeaf(std::uint16_t value) final {
            Container * container = new LeafArrayContainer();
            this->append(value, container);
            this->size_++;
            return container;
        }

        int indexOf(std::uint16_t index) const final {
            bool exists = this->bitset_.test(index);
            return (exists ? index : kInvalidIndex32);
        }

        Container * valueOf(int index) const final {
            bool exists = this->bitset_.test(index);
            if (exists)
                return this->valueArray_.valueOf(this->ptr_, index);
            else
                return nullptr;
        }
    };

    class LeafBitmapContainer final : public Container {
    private:
        std::bitset<kMaxArraySize>  bitset_;

        void init() {
            // Do nothing !!
        }

    public:
        LeafBitmapContainer() noexcept : Container(NodeType::LeafBitmapContainer, 0, kMaxArraySize, nullptr) {
            this->init();
        }
        LeafBitmapContainer(const LeafBitmapContainer & src) = delete;

        virtual ~LeafBitmapContainer() {
            this->destroy();
        }

        void destroy() final {
            // Do nothing !!
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
            // Do nothing !!
        }

        void resize(size_type newSize) final {
            // Do nothing !!
        }

        Container * getChild(std::uint16_t value) const final {
            // Do nothing !!
            return nullptr;
        }

        bool hasChild(std::uint16_t value, Container *& child) const final {
            child = nullptr;
            return this->hasLeaf(value);
        }

        bool hasLeaf(std::uint16_t value) const final {
            return this->bitset_.test(value);
        }

        void append(std::uint16_t value, Container * container) final {
            this->bitset_.set(value);
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

        int indexOf(std::uint16_t index) const final {
            bool exists = this->bitset_.test(index);
            return (exists ? index : kInvalidIndex32);
        }

        Container * valueOf(int index) const final {
            // Not supported, do nothing !!
            return nullptr;
        }
    };

    struct Factory {
        template <size_type type = NodeType::ArrayContainer>
        static Container * CreateNewContainer() {
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
                throw std::exception("Factory::CreateNewContainer<type>(): Unknown container type");
            }
            return container;
        }
    };

#pragma pack(pop)

private:
    Container *     root_;
    size_type       size_;
    size_type       y_index_[BoardY];
#if SPARSEBITSET_USE_TRIE_INFO
    LayerInfo       layer_info_[BoardY];
#endif

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
        this->create_root(NodeType::ArrayContainer);
    }

public:
    SparseBitset() : root_(nullptr), size_(0) {
        this->init();
    }

    ~SparseBitset() {
        this->destroy();
    }

    Container * root() {
        return this->root_;
    }

    const Container * root() const {
        return this->root_;
    }

    size_type size() const {
        return this->size_;
    }

    void destroy() {
        this->destroy_trie();
        this->size_ = 0;
    }

    Container * create_root(size_type type = NodeType::BitmapContainer) {
        Container * container = nullptr;
        if (this->root_ == nullptr) {
            if (type == NodeType::ArrayContainer)
                container = new ArrayContainer();
            else
                container = new BitmapContainer();
            this->root_ = container;
        }
        return container;
    }

    void shutdown() {
        this->destroy();
    }

    void destroy_trie_impl(Container * container, size_type layer) {
        assert(container != nullptr);
        for (size_type i = container->begin(); i < container->end(); container->next(i)) {
            Container * child = container->valueOf(i);
            assert(child != nullptr);
            if (child->type() != NodeType::LeafArrayContainer &&
                child->type() != NodeType::LeafBitmapContainer) {
                this->destroy_trie_impl(child, layer + 1);
            }
            delete child;
        }
    }

    void destroy_trie() {
        Container * container = this->root_;
        if (container == nullptr) {
            return;
        }

        for (size_type i = container->begin(); i < container->end(); container->next(i)) {
            Container * child = container->valueOf(i);
            if (child != nullptr) {
                this->destroy_trie_impl(child, 1);
                delete child;
            }
        }

        delete this->root_;
        this->root_ = nullptr;
    }

    void clear_trie_info() {
#if SPARSEBITSET_USE_TRIE_INFO
        for (size_type i = 0; i < BoardY; i++) {
            this->layer_info_[i].maxLayerSize = 0;
            this->layer_info_[i].childCount = 0;
            this->layer_info_[i].totalLayerSize = 0;
        }
#endif
    }

    size_type get_layer_value(const board_type & board, size_type layer) const {
        size_type y = this->y_index_[layer];
        ssize_type cell_y = y * BoardX;
        size_type layer_value = 0;
        for (ssize_type x = BoardX - 1; x >= 0; x--) {
            layer_value <<= 3;
            layer_value |= size_type(board.cells[cell_y + x] & kBitMask);
        }
        return layer_value;
    }

    void compose_segments_to_board(board_type & board, const std::uint16_t segment_list[BoardY]) {
        for (size_type segment = 0; segment < BoardY; segment++) {
            std::uint32_t value = (std::uint32_t)segment_list[segment];
            size_type y = this->y_index_[segment];
            size_type base_pos = y * BoardX;
            for (size_type x = 0; x < BoardX; x++) {
                std::uint32_t color = value & Color::Mask32;
                assert(color >= Color::First && color < Color::Maximum);
                size_type pos = base_pos + x;
                assert(pos < BoardSize);
                board.cells[pos] = (std::uint8_t)color;
                value >>= Color::Shift32;
            }
        }
    }

    bool contains(const board_type & board) const {
        Container * container = this->root_;
        assert(container != nullptr);
        // Normal container
        size_type layer;
        for (layer = 0; layer < BoardY - 1; layer++) {
            size_type layer_value = this->get_layer_value(board, layer);
            assert(container->type() == NodeType::ArrayContainer ||
                   container->type() == NodeType::BitmapContainer);
            Container * child;
            bool is_exists = container->hasChild(layer_value, child);
            if (is_exists) {
                assert(child != nullptr);
                container = child;
                continue;
            }
            else {
                return false;
            }
        }

        // Leaf container
        {
            size_type layer_value = this->get_layer_value(board, layer);
            assert(container->type() == NodeType::LeafArrayContainer ||
                   container->type() == NodeType::LeafBitmapContainer);
            bool is_exists = container->hasLeaf(layer_value);
            return is_exists;
        }
    }

    bool contains(const board_type & board, size_type & last_layer, Container *& last_container) const {
        Container * container = this->root_;
        assert(container != nullptr);
        // Normal container
        size_type layer;
        for (layer = 0; layer < BoardY - 1; layer++) {
            size_type layer_value = get_layer_value(board, layer);
            assert(container->type() == NodeType::ArrayContainer ||
                   container->type() == NodeType::BitmapContainer);
            Container * child;
            bool is_exists = container->hasChild(layer_value, child);
            if (is_exists) {
                assert(child != nullptr);
                container = child;
                continue;
            }
            else {
                last_layer = layer;
                last_container = container;
                return false;
            }
        }

        // Leaf container
        {
            size_type layer_value = this->get_layer_value(board, layer);
            assert(container->type() == NodeType::LeafArrayContainer ||
                   container->type() == NodeType::LeafBitmapContainer);
            bool is_exists = container->hasLeaf(layer_value);
            if (!is_exists) {
                last_layer = layer;
                last_container = container;
            }
            return is_exists;
        }
    }

    //
    // Root -> (ArrayContainer)0 -> (ArrayContainer)1 -> (ArrayContainer)2 -> (LeafArrayContainer)3 -> 4444
    //
    void append(const board_type & board) {
        Container * container = this->root_;
        assert(container != nullptr);
        bool insert_new = false;
        // Normal container
        size_type layer;
        for (layer = 0; layer < BoardY - 1; layer++) {
            size_type layer_value = this->get_layer_value(board, layer);
            if (!insert_new) {
                assert(container->type() == NodeType::ArrayContainer ||
                       container->type() == NodeType::BitmapContainer);
                Container * child;
                bool is_exists = container->hasChild(layer_value, child);
                if (is_exists) {
                    assert(child != nullptr);
                    container = child;
                    continue;
                }
                else {
                    insert_new = true;
                }
            }
            if (layer < (BoardY - 2))
                container = container->append(layer_value);
            else
                container = container->appendLeaf(layer_value);
        }

        // Leaf container
        {
            size_type layer_value = this->get_layer_value(board, layer);
            assert(container->type() == NodeType::LeafArrayContainer ||
                   container->type() == NodeType::LeafBitmapContainer);
            if (!insert_new) {
                bool is_exists = container->hasLeaf(layer_value);
                if (is_exists) {
                    return;
                }
            }
            container->appendLeaf(layer_value);
            this->size_++;
        }
    }

    //
    // Root -> (ArrayContainer)0 -> (ArrayContainer)1 -> (ArrayContainer)2 -> (LeafArrayContainer)3 -> 4444
    //
    bool try_append(const board_type & board) {
        Container * container = this->root_;
        assert(container != nullptr);
        bool insert_new = false;
        // Normal container
        size_type layer;
        for (layer = 0; layer < BoardY - 1; layer++) {
            size_type layer_value = this->get_layer_value(board, layer);
            if (!insert_new) {
                assert(container->type() == NodeType::ArrayContainer ||
                       container->type() == NodeType::BitmapContainer);
                Container * child;
                bool is_exists = container->hasChild(layer_value, child);
                if (is_exists) {
                    assert(child != nullptr);
                    container = child;
                    continue;
                }
                else {
                    insert_new = true;
                }
            }
            if (layer < (BoardY - 2))
                container = container->append(layer_value);
            else
                container = container->appendLeaf(layer_value);
        }

        // Leaf container
        {
            size_type layer_value = this->get_layer_value(board, layer);
            assert(container->type() == NodeType::LeafArrayContainer ||
                   container->type() == NodeType::LeafBitmapContainer);
            if (!insert_new) {
                bool is_exists = container->hasLeaf(layer_value);
                if (is_exists) {
                    return false;
                }
            }
            container->appendLeaf(layer_value);
            this->size_++;
        }

        return true;
    }

    void append_new(const board_type & board, size_type last_layer, Container * last_container) {
        Container * container = last_container;
        assert(container != nullptr);
        // Normal container
        size_type layer;
        for (layer = last_layer; layer < BoardY - 1; layer++) {
            size_type layer_value = this->get_layer_value(board, layer);
            if (layer < (BoardY - 2))
                container = container->append(layer_value);
            else
                container = container->appendLeaf(layer_value);
        }

        // Leaf container
        {
            assert(container != nullptr);
            size_type layer_value = this->get_layer_value(board, layer);
            assert(container->type() == NodeType::LeafArrayContainer ||
                   container->type() == NodeType::LeafBitmapContainer);
            container->appendLeaf(layer_value);
        }

        this->size_++;
    }

    bool remove(const board_type & board) {
        return true;
    }

    void count_trie_info_impl(Container * container, size_type layer) {
#if SPARSEBITSET_USE_TRIE_INFO
        assert(container != nullptr);
        size_type layer_size = container->size();
        if (layer_size > this->layer_info_[layer].maxLayerSize) {
            this->layer_info_[layer].maxLayerSize = layer_size;
        }
        this->layer_info_[layer].childCount++;
        this->layer_info_[layer].totalLayerSize += layer_size;

        if (container->type() == NodeType::LeafArrayContainer ||
            container->type() == NodeType::LeafBitmapContainer) {
            return;
        }

        for (size_type i = container->begin(); i < container->end(); container->next(i)) {
            Container * child = container->valueOf(i);
            assert(child != nullptr);
            this->count_trie_info_impl(child, layer + 1);
        }
#endif
    }

    void count_trie_info() {
#if SPARSEBITSET_USE_TRIE_INFO
        this->clear_trie_info();

        Container * container = this->root_;
        if (container == nullptr) {
            return;
        }

        size_type layer_size = container->size();
        if (layer_size > this->layer_info_[0].maxLayerSize) {
            this->layer_info_[0].maxLayerSize = layer_size;
        }
        this->layer_info_[0].childCount++;
        this->layer_info_[0].totalLayerSize += layer_size;

        for (size_type i = container->begin(); i < container->end(); container->next(i)) {
            Container * child = container->valueOf(i);
            if (child != nullptr) {
                this->count_trie_info_impl(child, 1);
            }
        }
#endif
    }

    void display_trie_info() {
#if SPARSEBITSET_USE_TRIE_INFO
        this->count_trie_info();

        printf("SparseBitset<T> trie info:\n\n");
        for (size_type i = 0; i < BoardY; i++) {
            printf("[%u]: maxLayerSize = %8u, childCount = %8u, averageSize = %0.2f\n\n",
                   uint32_t(i + 1),
                   uint32_t(this->layer_info_[i].maxLayerSize),
                   uint32_t(this->layer_info_[i].childCount),
                   (double)this->layer_info_[i].totalLayerSize / this->layer_info_[i].childCount);
            if (i == BoardY - 1) {
                printf("[%u]: maxLayerSize = %8u, childCount = %8u, averageSize = %0.2f\n\n",
                       uint32_t(i + 2),
                       uint32_t(0),
                       uint32_t(this->layer_info_[i].totalLayerSize),
                       (double)0.0);
            }
        }
        printf("\n");
#endif
    }
};

} // namespace AI
} // namespace MagicBlock
