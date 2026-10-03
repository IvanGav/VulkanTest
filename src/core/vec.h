#pragma once

#include "prelude.h"
#include "mem.h"
#include "slice.h"

template <typename T>
struct Vec {
    T* data; // nullable, owned
    u32 size;
    u32 capacity;

    mem::Arena* arena;

    template <typename... Args>
    static Vec with(mem::Arena* arena, Args&&... args) {
        Vec<T> v { .arena = arena };
        v.reserve(next_power_of_two(sizeof...(args)));
        (v.push(std::forward<Args>(args)), ...);
        return v;
    }

    static Vec with_capacity(mem::Arena* arena, u32 capacity) {
        Vec<T> v{ .arena = arena };
        v.reserve(capacity);
        return v;
    }

    static Vec clone_slice(mem::Arena* arena, Slice<T>& to_clone) {
        Vec<T> v { .arena = arena };
        v.resize(to_clone.size);
        mem::copy<T>(to_clone.data, v.data, to_clone.size);
    }

    void reserve(u32 capacity) {
        mem::Arena* arena = this->arena;
        if(arena == nullptr) arena = &global_arena;
        data = arena->realloc(data, this->capacity, capacity);
        this->capacity = capacity;
    }

    void resize(u32 size) {
        if(size > capacity) this->reserve(size);
        if(size > this->size)
            mem::zero<T>(data + this->size, size - this->size);
        this->size = size;
    }

    void reserve() {
        if(size == capacity) {
            if(capacity == 0) this->reserve(8);
            else this->reserve(capacity*2);
        }
    }
    
    bool is_empty() {
        return size == 0;
    }

    void push(T e) {
        this->reserve();
        data[size] = std::move(e);
        size++;
    }

    // insert `e` at `index`
    void insert(u32 index, T const& e) {
        this->reserve();
        for(u32 i = size; i > index; i--) {
            data[i] = data[i-1];
        }
        data[index] = e;
        size++;
    }

    void swap(u32 i1, u32 i2) {
        mem::swap<T>(data[i1], data[i2]);
    }

    T pop() {
        assert(size > 0);
        size--;
        return data[size];
    }

    void push_slice(Slice<T> s) {
        if(next_power_of_two(this->size + s.size) < this->capacity)
            this->reserve(next_power_of_two(this->size + s.size));
        
        for(u32 i = 0; i < s.size; i++) {
            this->push(s[i]);
        }
    }

    // set size to 0 and make sure that new elements get put into new memory
    void invalidate() {
        size = 0;
        capacity = 0;
        data = nullptr;
    }

    /* Access Member Functions */

    T const& operator[](u32 i) const {
        assert(i < size);
        return data[i];
    }

    T const& front() const {
        assert(size > 0);
        return data[0];
    }

    T const& back() const {
        assert(size > 0);
        return data[size-1];
    }

    T& operator[](u32 i) {
        if (i >= size) { __debugbreak(); }
        assert(i < size);
        return data[i];
    }

    T& front() {
        assert(size > 0);
        return data[0];
    }

    T& back() {
        assert(size > 0);
        return data[size-1];
    }

    /* Util functions */

    void reverse() {
        u32 size = this->size;
        u32 midpoint = size/2;
        for(u32 i = 0; i < midpoint; i++) {
            mem::swap(this->data[i], this->data[size-1-i]);
        }
    }

    void clear() {
        size = 0;
    }

    // Remove an element at a given index. Order preserving, O(n).
    void remove(u32 remove_index) {
        assert(remove_index < size);
        size--;
        for(u32 i = remove_index; i < size; i++) {
            data[i] = data[i+1];
        }
    }

    // Remove an element at a given index. Does not preserve order, but O(1).
    void remove_swap(u32 remove_index) {
        assert(remove_index < size);
        size--;
        data[remove_index] = data[size];
    }

    // Remove a single element by value. Return true if something was deleted. Order preserving, O(n).
    bool remove_first_of(T const& e) {
        u32 i = this->index_of(e);
        if(i == size) return false;
        this->remove(i);
        return true;
    }

    // Remove a single element by value. Return true if something was deleted. Does not preserve order, but O(1).
    bool remove_swap_first_of(T const& e) {
        u32 i = this->index_of(e);
        if(i == size) return false;
        this->remove_swap(i);
        return true;
    }

    // Return the index of the first occurance of `e` or `size` if not found
    u32 index_of(T const& e) const {
        for(u32 i = 0; i < size; i++) {
            if(data[i] == e) {
                return i;
            }
        }
        return size;
    }

    // Return true if this vector contains a given element
    bool contains(T const& e) const {
        for(u32 i = 0; i < size; i++) {
            if(data[i] == e) {
                return true;
            }
        }
        return false;
    }

    /* Slice compatibility */

    Slice<T> full_slice() {
        return Slice<T> { .data = data, .size = size };
    }
    Slice<T> slice(u32 start, u32 size) {
        assert(start+size <= this->size);
        return Slice<T> { .data = data + start, .size = size };
    }
    // `end` exclusive
    Slice<T> slice_range(u32 start, u32 end) {
        assert(end <= this->size);
        return Slice<T> { .data = data + start, .size = size };
    }

    /* Cloning */

    // shallow clone
    Vec<T> clone(mem::Arena* arena = nullptr) {
        if(arena == nullptr) arena = this->arena;
        if(arena == nullptr) arena = global_arena;
        Vec<T> cloned {
            .data = arena->alloc<T>(capacity),
            .size = size,
            .capacity = capacity,
            .arena = arena,
        };
        mem::copy<T>(cloned.data, data, size);
        return cloned;
    }

    /* STL Compatibility */

    T* begin() {
        return data;
    }
    T* end() {
        return data + size;
    }

    bool operator==(const Vec<T>& other) const {
        if(size != other.size) return false;
        for(u32 i = 0; i < size; i++) {
            if(data[i] != other.data[i]) return false;
        }
        return true;
    }
};

template <typename T>
std::ostream& operator<<(std::ostream& os, Vec<T>& vec) {
    os << '[';
    for(T& e : vec) {
        os << e << ',';
    }
    os << ']';
    return os;
}