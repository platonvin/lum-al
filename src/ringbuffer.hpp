#pragma once

/*
array with built-in index_counter that increments on move() and wraps around size
*/

#include <algorithm>
#include <cstddef>

template <typename _Type>
class ring {
public:
    ring(size_t initialSize) {
        buffer = new _Type[initialSize];
        size_ = initialSize;
        current_index = 0;
    }
    ring(const ring& other) {
        size_ = other.size_;
        current_index = other.current_index;
        buffer = new _Type[size_];
        std::copy(other.buffer, other.buffer + size_, buffer);
    }
    ring() {
        buffer = NULL;
        size_ = 0;
        current_index = 0;
    }
    ring(std::initializer_list<_Type> initList)
        : buffer(new _Type[initList.size()]), size_(initList.size()), current_index(0) {
        std::copy(initList.begin(), initList.end(), buffer);
    }

    ~ring() {
        delete[] buffer;
    }

    _Type* data(){
        return buffer;
    }

    void allocate(size_t size) {
        if (buffer) {
            delete[] buffer;
        }
        buffer = new _Type[size];
        size_ = size;
        current_index = 0;
    }

    //basically for changing FIF count
    void realloc(size_t newSize) {
        if (newSize == size_) return;

        _Type* newBuffer = new _Type[newSize];
        size_t elementsToCopy = (newSize < size_) ? newSize : size_;

        for (size_t i = 0; i < elementsToCopy; ++i) {
            newBuffer[i] = buffer[i];
        }

        delete[] buffer;
        buffer = newBuffer;
        size_ = newSize;
        if (current_index >= size_) {
            current_index = 0;
        }
    }

    _Type& next() {
        size_t nextIndex = (current_index + 1) % size_;
        return buffer[nextIndex];
    }
    _Type& previous() {
        size_t previousIndex = ((size_ + current_index) - 1) % size_;
        return buffer[previousIndex];
    }
    _Type& current() {
        return buffer[current_index];
    }

    void reset() {current_index = 0;}
    void move() {current_index = (current_index + 1) % size_;}

    size_t size() {return size_;}

    bool empty(){
        return size_ == 0;
    }

    ring& operator=(const ring& other) {
        if (this != &other) {
            delete[] buffer;
            size_ = other.size_;
            current_index = other.current_index;
            buffer = new _Type[size_];
            std::copy(other.buffer, other.buffer + size_, buffer);
        }
        return *this;
    }
    ring(ring&& other) {
        buffer = other.buffer;
        size_ = other.size_;
        current_index = other.current_index;
        other.buffer = nullptr;
        other.size_ = 0;
        other.current_index = 0;
    }
    ring& operator=(ring&& other) {
        if (this != &other) {
            delete[] buffer;
            buffer = other.buffer;
            size_ = other.size_;
            current_index = other.current_index;
            other.buffer = nullptr;
            other.size_ = 0;
            other.current_index = 0;
        }
        return *this;
    }
    _Type& operator[](size_t index) {
        return buffer[index];
    }

    class iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = _Type;
        using pointer = _Type*;
        using reference = _Type&;

        iterator(pointer ptr): ptr(ptr) {}

        reference operator*() const {return *ptr;}
        pointer operator->() {return ptr;}
        iterator& operator++() { ptr++; return *this; }  
        iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

        friend bool operator== (const iterator& a, const iterator& b) { return a.ptr == b.ptr; };
        friend bool operator!= (const iterator& a, const iterator& b) { return a.ptr != b.ptr; }; 

        private:
            pointer ptr;
    };

    iterator begin() {
        return iterator(&buffer[0]);
    }

    iterator end() {
        return iterator(&buffer[size_]); //not size_ -1
    }
    
private:
    _Type* buffer = NULL;
    size_t size_ = 0;
    size_t current_index = 0;
};