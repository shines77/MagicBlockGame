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

#include "Value128.h"

#define SPARSEBITSET_DISPLAY_TRIE_INFO  0

namespace MagicBlock {

namespace Algorithm {
    static void merge_sort(std::uint16_t * indexs, std::uint16_t * new_indexs,
                           std::size_t first, std::size_t last) {
        assert(indexs != nullptr);
        assert(new_indexs != nullptr);
        assert(first < last);
        std::size_t left = first;
        std::size_t middle = (first + last) / 2;
        std::size_t right = middle;
        std::size_t cur = first;
        while (left < middle && right < last) {
            if (indexs[left] <= indexs[right])
                new_indexs[cur++] = indexs[left++];
            else
                new_indexs[cur++] = indexs[right++];
        }

        while (left < middle) {
            new_indexs[cur++] = indexs[left++];
        }

        while (right < last) {
            new_indexs[cur++] = indexs[right++];
        }
    }

    static void merge_sort(std::uint16_t * indexs, std::uintptr_t ** values,
                           std::uint16_t * new_indexs, std::uintptr_t ** new_values,
                           std::size_t first, std::size_t last) {
        assert(indexs != nullptr);
        assert(new_indexs != nullptr);
        assert(values != nullptr);
        assert(new_values != nullptr);
        assert(first < last);
        std::size_t left = first;
        std::size_t middle = (first + last) / 2;
        std::size_t right = middle;
        std::size_t cur = first;
        while (left < middle && right < last) {
            if (indexs[left] <= indexs[right]) {
                new_indexs[cur] = indexs[left];
                new_values[cur] = values[left];
                cur++;
                left++;
            }
            else {
                new_indexs[cur] = indexs[right];
                new_values[cur] = values[right];
                cur++;
                right++;
            }
            assert(cur <= last);
        }

        while (left < middle) {
            new_indexs[cur] = indexs[left];
            new_values[cur] = values[left];
            cur++;
            left++;
            assert(cur <= last);
        }

        while (right < last) {
            new_indexs[cur] = indexs[right];
            new_values[cur] = values[right];
            cur++;
            right++;
            assert(cur <= last);
        }
    }

#if 1
    //
    // See: https://www.cnblogs.com/skywang12345/p/3596746.html
    // See: https://blog.csdn.net/zuiaituantuan/article/details/5978009 (recommend)
    //

    // Prerequisite: all elements are unique
    static void quick_sort(std::uint16_t * indexs, std::ptrdiff_t first, std::ptrdiff_t last) {
        assert(indexs != nullptr);
        if (first < last) {
            std::ptrdiff_t left = first;
            std::ptrdiff_t right = last;
            std::uint16_t pivot = indexs[left];
            while (left < right) {
                while (left < right && indexs[right] > pivot) {
                    right--;
                }
                if (left < right) {
                    indexs[left++] = indexs[right];
                }
                while (left < right && indexs[left] < pivot) {
                    left++;
                }
                if (left < right) {
                    indexs[right--] = indexs[left];
                }
            }
            indexs[left] = pivot;

            quick_sort(indexs, first, left - 1);
            quick_sort(indexs, left + 1, last);
        }
    }

    // Prerequisite: all elements are unique
    static void quick_sort(std::uint16_t * indexs, std::uintptr_t ** values,
                           std::ptrdiff_t first, std::ptrdiff_t last) {
        assert(indexs != nullptr);
        assert(values != nullptr);
        if (first < last) {
            std::ptrdiff_t left = first;
            std::ptrdiff_t right = last;
            std::uint16_t pivot = indexs[left];
            std::uintptr_t * pivot_value = values[left];
            while (left < right) {
                while (left < right && indexs[right] > pivot) {
                    right--;
                }
                if (left < right) {
                    indexs[left] = indexs[right];
                    values[left] = values[right];
                    left++;
                }
                while (left < right && indexs[left] < pivot) {
                    left++;
                }
                if (left < right) {
                    indexs[right] = indexs[left];
                    values[right] = values[left];
                    right--;
                }
            }
            indexs[left] = pivot;
            values[left] = pivot_value;

            quick_sort(indexs, values, first, left - 1);
            quick_sort(indexs, values, left + 1, last);
        }
    }
#else
    //
    // See: https://baijiahao.baidu.com/s?id=1667443239764190065
    // See: https://blog.csdn.net/zuiaituantuan/article/details/5978009 (recommend)
    //

    // Prerequisite: all elements are unique
    static void quick_sort(std::uint16_t * indexs, std::ptrdiff_t first, std::ptrdiff_t last) {
        assert(indexs != nullptr);
        if (first < last) {
            std::ptrdiff_t left = first;
            std::ptrdiff_t right = last;
            std::size_t middle = (left + right) / 2;
            std::swap(indexs[left], indexs[middle]);
            std::uint16_t pivot = indexs[left];
            while (left < right) {
                while (left < right && indexs[right] > pivot) {
                    right--;
                }
                if (left < right) {
                    indexs[left++] = indexs[right];
                }
                while (left < right && indexs[left] < pivot) {
                    left++;
                }
                if (left < right) {
                    indexs[right--] = indexs[left];
                }
            }
            indexs[left] = pivot;

            quick_sort(indexs, first, left - 1);
            quick_sort(indexs, left + 1, last);
        }
    }

    // Prerequisite: all elements are unique
    static void quick_sort(std::uint16_t * indexs, std::uintptr_t ** values,
                           std::ptrdiff_t first, std::ptrdiff_t last) {
        assert(indexs != nullptr);
        assert(values != nullptr);
        if (first < last) {
            std::ptrdiff_t left = first;
            std::ptrdiff_t right = last;
            std::ptrdiff_t middle = (left + right) / 2;
            std::swap(indexs[left], indexs[middle]);
            std::swap(values[left], values[middle]);
            std::uint16_t pivot = indexs[left];
            std::uintptr_t * pivot_value = values[left];
            while (left < right) {
                while (left < right && indexs[right] > pivot) {
                    right--;
                }
                if (left < right) {
                    indexs[left] = indexs[right];
                    values[left] = values[right];
                    left++;
                }
                while (left < right && indexs[left] < pivot) {
                    left++;
                }
                if (left < right) {
                    indexs[right] = indexs[left];
                    values[right] = values[left];
                    right--;
                }
            }
            indexs[left] = pivot;
            values[left] = pivot_value;

            quick_sort(indexs, values, first, left - 1);
            quick_sort(indexs, values, left + 1, last);
        }
    }
#endif

#if 1
    static int binary_search(std::uint16_t * ptr, std::size_t first,
                             std::size_t last, std::uint16_t value) {
        std::size_t low = first;
        std::size_t high = last;

        //while (std::ptrdiff_t(high - low) >= 8) {
        while (low < high) {
            std::size_t mid = (low + high) / 2;
            std::uint16_t middle = ptr[mid];
            if (value < middle)
                high = mid;
            else if (value > middle)
                low = mid + 1;
            else
                return (int)mid;
        }
#if 0
        std::uint16_t * indexFirst = (std::uint16_t *)ptr + low;
        std::uint16_t * indexLast  = (std::uint16_t *)ptr + high;
        for (std::uint16_t * indexs = indexFirst; indexs < indexLast; indexs++) {
            assert(*indexs != std::uint16_t(-1));
            if (*indexs != value)
                continue;
            else
                return int(indexs - (std::uint16_t *)ptr);
        }
#endif
        return -1;
    }
#else
    static int binary_search(std::uint16_t * ptr, std::size_t first,
                             std::size_t last, std::uint16_t value) {
        std::size_t low = first;
        std::size_t high = last;

        assert(low < high);
        std::uint16_t maximum = ptr[high - 1];
        if (value < maximum) {
            std::uint16_t minimum = ptr[low];
            if (value > minimum) {
                //while (std::ptrdiff_t(high - low) >= 8) {
                while (low < high) {
                    std::size_t mid = (low + high) / 2;
                    std::uint16_t middle = ptr[mid];
                    if (value < middle)
                        high = mid;
                    else if (value > middle)
                        low = mid + 1;
                    else
                        return (int)mid;
                }
#if 0
                std::uint16_t * indexFirst = (std::uint16_t *)ptr + low;
                std::uint16_t * indexLast  = (std::uint16_t *)ptr + high;
                for (std::uint16_t * indexs = indexFirst; indexs < indexLast; indexs++) {
                    assert(*indexs != std::uint16_t(-1));
                    if (*indexs != value)
                        continue;
                    else
                        return int(indexs - (std::uint16_t *)ptr);
                }
#endif
            }
            else if (value == minimum) {
                return (int)low;
            }
        }
        else if (value == maximum) {
            return (int)(high - 1);
        }
        return -1;
    }
#endif
} // namespace Algorithm

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
        IndexVector() = default;
        ~IndexVector() = default;

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
        ValueVector() = default;
        ~ValueVector() = default;

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
        IndexArray() = default;
        ~IndexArray() = default;

        size_type size() const {
            return 0;
        }

        void reserve(size_type capacity) {
        }

        void resize(size_type newSize) {
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
            if (sorted > 0) {
                int index = Algorithm::binary_search((std::uint16_t *)ptr, 0, sorted, value);
                if (index != kInvalidIndex32)
                    return index;
            }

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
        }

        void append(std::uintptr_t * ptr, std::uint16_t size, std::uint16_t value) {
            assert(size <= kArraySizeThreshold);
            assert(size <= kMaxArraySize);
            std::uint16_t * target = (std::uint16_t *)ptr + size;
            assert(target != nullptr);
            *target = value;
        }
    };

    class ValueArray {
    public:
        ValueArray() = default;
        ~ValueArray() = default;

        size_type size() const {
            return 0;
        }

        void reserve(size_type capacity) {
        }

        void resize(size_type newSize) {
        }

        Container * valueOf(std::uintptr_t * ptr, std::uint16_t capacity, int index) const {
            assert(index != kInvalidIndex32);
            assert(index >= 0 && index < (int)capacity);
            assert(capacity <= std::uint16_t(kMaxArraySize));
            std::uint16_t * indexEnd = (std::uint16_t *)ptr + capacity;
            Container ** value = (Container **)indexEnd + index;
            return *value;
        }

        void append(std::uintptr_t * ptr, std::uint16_t size, std::uint16_t capacity, Container * container) {
            assert(container != nullptr);
            assert(size <= std::uint16_t(kArraySizeThreshold));
            assert(size <= std::uint16_t(kMaxArraySize));
            assert(size <= capacity);
            std::uint16_t * indexEnd = (std::uint16_t *)ptr + capacity;
            Container ** value = (Container **)indexEnd + size;
            assert(value != nullptr);
            *value = container;
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
        Container() : type_(NodeType::ArrayContainer), size_(0), capacity_(0), sorted_(0), ptr_(nullptr) {
            this->init();
        }
        Container(std::uint16_t type) : type_(type), size_(0), capacity_(0), sorted_(0), ptr_(nullptr) {
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

        virtual Container * hasChild(std::uint16_t value) const {
            // Not implemented!
            return nullptr;
        }

        Container * hasChild(std::size_t value) const {
            return this->hasChild(static_cast<std::uint16_t>(value));
        }

        virtual bool hasChild(std::uint16_t value, Container *& container) const {
            // Not implemented!
            return false;
        }

        bool hasChild(std::size_t value, Container *& container) const {
            return this->hasChild(static_cast<std::uint16_t>(value), container);
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
        ArrayContainer() : Container(NodeType::ArrayContainer) {
            this->init();
        }
        ArrayContainer(const ArrayContainer & src) = delete;

        virtual ~ArrayContainer() {
            this->destroy();
        }

        void destroy() final {
            // TODO:
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

        Container * hasChild(std::uint16_t value) const final {
            int index = this->indexArray_.indexOf(this->ptr_, this->size_, this->sorted_, value);
            if (index != kInvalidIndex32) {
                Container * child = this->valueArray_.valueOf(this->ptr_, this->capacity_, index);
                return child;
            }
            return nullptr;
        }

        bool hasChild(std::uint16_t value, Container *& container) const final {
            int index = this->indexArray_.indexOf(this->ptr_, this->size_, this->sorted_, value);
            if (index != kInvalidIndex32) {
                Container * child = this->valueArray_.valueOf(this->ptr_, this->capacity_, index);
                assert(child != nullptr);
                container = child;
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

        Container * valueOf(int index) const final {
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
        LeafArrayContainer() : Container(NodeType::LeafArrayContainer) {
            this->init();
        }
        LeafArrayContainer(const LeafArrayContainer & src) = delete;

        virtual ~LeafArrayContainer() {
            this->destroy();
        }

        void destroy() final {
            // TODO:
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

        Container * hasChild(std::uint16_t value) const final {
            return nullptr;
        }

        bool hasChild(std::uint16_t value, Container *& container) const final {
            container = nullptr;
            return this->hasLeaf(value);;
        }

        bool hasLeaf(std::uint16_t value) const final {
            int index = indexArray_.indexOf(this->ptr_, this->size_, this->sorted_, value);
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

        Container * valueOf(int index) const final {
            return nullptr;
        }
    };

    class BitmapContainer final : public Container {
    public:
        BitmapContainer() : Container(NodeType::BitmapContainer) {
            this->init();
        }
        BitmapContainer(const BitmapContainer & src) = delete;

        virtual ~BitmapContainer() {
            this->destroy();
        }

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

        bool hasChild(std::uint16_t value, Container *& container) const final {
            // TODO:
            return false;
        }

        bool hasLeaf(std::uint16_t value) const final {
            return false;
        }

        void append(std::uint16_t value, Container * container) final {
            // TODO:
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

        Container * valueOf(int index) const final {
            // TODO:
            return nullptr;
        }
    };

    class LeafBitmapContainer final : public Container {
    public:
        LeafBitmapContainer() : Container(NodeType::LeafBitmapContainer) {
            this->init();
        }
        LeafBitmapContainer(const LeafBitmapContainer & src) = delete;

        virtual ~LeafBitmapContainer() {
            this->destroy();
        }

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

        bool hasChild(std::uint16_t value, Container *& container) const final {
            container = nullptr;
            return this->hasLeaf(value);
        }

        bool hasLeaf(std::uint16_t value) const final {
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
        this->root_ = new ArrayContainer();

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

    void destroy_trie_impl(Container * container, size_type layer) {
        assert(container != nullptr);
#if SPARSEBITSET_DISPLAY_TRIE_INFO
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
#endif
        for (size_type i = container->begin(); i < container->end(); container->next(i)) {
            Container * child = container->valueOf(i);
            if (child != nullptr) {
#if (SPARSEBITSET_DISPLAY_TRIE_INFO == 0)
                if (child->type() != NodeType::LeafArrayContainer &&
                    child->type() != NodeType::LeafBitmapContainer) {
                    destroy_trie_impl(child, layer + 1);
                }
#else
                destroy_trie_impl(child, layer + 1);
#endif
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
                delete child;
            }
        }

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

    size_type getLayerValue(const board_type & board, size_type layer) const {
        size_type y = this->y_index_[layer];
        ssize_type cell_y = y * BoardY;
        size_type layer_value = 0;
        for (ssize_type x = BoardX - 1; x >= 0; x--) {
            layer_value <<= 3;
            layer_value |= size_type(board.cells[cell_y + x] & kBitMask);
        }
        return layer_value;
    }

    bool contains(const board_type & board) const {
        Container * container = this->root_;
        assert(container != nullptr);
        // Normal container
        size_type layer;
        for (layer = 0; layer < BoardY - 1; layer++) {
            size_type layer_value = getLayerValue(board, layer);
            assert(container->type() == NodeType::ArrayContainer ||
                   container->type() == NodeType::BitmapContainer);
            Container * child;
            bool is_exists = container->hasChild(layer_value, child);
            if (is_exists) {
                if (child != nullptr) {
                    assert(child->type() == NodeType::ArrayContainer ||
                           child->type() == NodeType::BitmapContainer ||
                           child->type() == NodeType::LeafArrayContainer ||
                           child->type() == NodeType::LeafBitmapContainer);
                    container = child;
                }
                else {
                    assert(container->isLeaf());
                }
                continue;
            }
            else {
                return false;
            }
        }

        // Leaf container
        {
            size_type layer_value = getLayerValue(board, layer);
            assert(container->type() == NodeType::LeafArrayContainer ||
                   container->type() == NodeType::LeafBitmapContainer);
            bool is_exists = container->hasLeaf(layer_value);
            return is_exists;
        }
    }

    //
    // Root -> (ArrayContainer)0 -> (ArrayContainer)1 -> (ArrayContainer)2 -> (LeafArrayContainer)3 -> 4444
    //
    bool append(const board_type & board) {
        Container * container = this->root_;
        assert(container != nullptr);
        bool insert_new = false;
        // Normal container
        size_type layer;
        for (layer = 0; layer < BoardY - 1; layer++) {
            size_type layer_value = getLayerValue(board, layer);
            if (!insert_new) {
                assert(container->type() == NodeType::ArrayContainer ||
                       container->type() == NodeType::BitmapContainer);
                Container * child;
                bool is_exists = container->hasChild(layer_value, child);
                if (is_exists) {
                    if (child != nullptr)
                        container = child;
                    else
                        assert(container->isLeaf());
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
            size_type layer_value = getLayerValue(board, layer);
            assert(container->type() == NodeType::LeafArrayContainer ||
                   container->type() == NodeType::LeafBitmapContainer);
            if (!insert_new) {
                bool is_exists = container->hasLeaf(layer_value);
                if (is_exists) {
                    return false;
                }
            }
            container->appendLeaf(layer_value);
        }

        if (insert_new) {
            this->size_++;
        }

        return insert_new;
    }

    bool remove(const board_type & board) {
        return true;
    }
};

} // namespace MagicBlock
