#pragma once

#include "prelude.h"
#include "mem.h"
#include "slice.h"

template <typename T, u32 N>
struct TinyVec {
    union {
        T* data; // nullable, owned
        T tiny_data[N];
    };
    u32 size;
    u32 capacity;
    
    mem::Arena* arena;

    template <typename... Args>
    static TinyVec with(mem::Arena* arena, Args&&... args) {
        TinyVec<T,N> v { .arena = arena };
        v.reserve(next_power_of_two(sizeof...(args)));
        (v.push(std::forward<Args>(args)), ...);
        return v;
    }

    template <u32 C>
    static TinyVec clone_slice(mem::Arena* arena, Slice<T>& to_clone) {
        TinyVec<T,C> v { arena };
        v.reserve(to_clone.size);
        for(T& e : to_clone) {
            v.push(e);
        }
        return v;
    }

    void reserve(u32 capacity) {
        mem::Arena* arena = this->arena;
        if(arena == nullptr) arena = &global_arena;
        // 4 scenarios: tiny->tiny, tiny->big, big->tiny, big->big
        if(this->capacity <= N) {
            if(capacity <= N) {
            } else {
                T* data = arena->alloc<T>(capacity);
                mem::copy(tiny_data, data, size);
                this->data = data;
            }
        } else {
            if(capacity <= N) {
                T* data = this->data;
                mem::copy(data, tiny_data, min(size, N));
            } else {
                data = arena->realloc<T>(data, this->capacity, capacity);
            }
        }
        this->capacity = capacity;
    }

    // void resize(u32 size) {
    //     if(size > capacity) this->reserve(size);
    //     if(size > this->size)
    //         mem::zero<T>(this->data + this->size, size - this->size);
    //     this->size = size;
    // }

    void reserve() {
        if(capacity < N) {
            capacity = N;
        } else if(size == capacity) {
            if(capacity == 0) this->reserve(N);
            else this->reserve(capacity*2);
        }
    }
    
    bool is_empty() {
        return size == 0;
    }

    void push(T e) {
        this->reserve();
        if(capacity <= N) tiny_data[size] = e;
        else data[size] = e;
        size++;
    }

    // insert `e` at `index`
    // void insert(u32 index, T const& e) {
    //     this->reserve();
    //     for(u32 i = size; i > index; i--) {
    //         data[i] = data[i-1];
    //     }
    //     data[index] = e;
    //     size++;
    // }

    void swap(u32 i1, u32 i2) {
        if(capacity <= N) {
            mem::swap<T>(tiny_data[i1], tiny_data[i2]);
        } else {
            mem::swap<T>(data[i1], data[i2]);
        }
    }

    T pop() {
        assert(size > 0);
        size--;
        if(capacity <= N) {
            return tiny_data[size];
        } else {
            return data[size];
        }
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
        if(capacity <= N) {
            return tiny_data[i];
        } else {
            return data[i];
        }
    }

    T const& front() const {
        assert(size > 0);
        if(capacity <= N) {
            return tiny_data[0];
        } else {
            return data[0];
        }
    }

    T const& back() const {
        assert(size > 0);
        if(capacity <= N) {
            return tiny_data[size-1];
        } else {
            return data[size-1];
        }
    }

    T& operator[](u32 i) {
        assert(i < size);
        if(capacity <= N) {
            return tiny_data[i];
        } else {
            return data[i];
        }
    }

    T& front() {
        assert(size > 0);
        if(capacity <= N) {
            return tiny_data[0];
        } else {
            return data[0];
        }
    }

    T& back() {
        assert(size > 0);
        if(capacity <= N) {
            return tiny_data[size-1];
        } else {
            return data[size-1];
        }
    }

    /* Util functions */

    void reverse() {
        u32 size = this->size;
        u32 midpoint = size/2;
        if(capacity <= N) {
            for(u32 i = 0; i < midpoint; i++) {
                mem::swap(this->tiny_data[i], this->tiny_data[size-1-i]);
            }
        } else {
            for(u32 i = 0; i < midpoint; i++) {
                mem::swap(this->data[i], this->data[size-1-i]);
            }
        }
    }

    void clear() {
        size = 0;
    }

    // Remove an element at a given index. Order preserving, O(n).
    void remove(u32 remove_index) {
        assert(remove_index < size);
        size--;
        if(capacity <= N) {
            for(u32 i = remove_index; i < size; i++) {
                tiny_data[i] = tiny_data[i+1];
            }
        } else {
            for(u32 i = remove_index; i < size; i++) {
                data[i] = data[i+1];
            }
        }
    }

    // Remove an element at a given index. Does not preserve order, but O(1).
    void remove_swap(u32 remove_index) {
        assert(remove_index < size);
        size--;
        if(capacity <= N) {
            tiny_data[remove_index] = tiny_data[size];
        } else {
            data[remove_index] = data[size];
        }
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
        if(capacity <= N) {
            for(u32 i = 0; i < size; i++) {
                if(tiny_data[i] == e) {
                    return i;
                }
            }
        } else {
            for(u32 i = 0; i < size; i++) {
                if(data[i] == e) {
                    return i;
                }
            }
        }
        return size;
    }

    // Return true if this TinyVector contains a given element
    bool contains(T const& e) const {
        if(capacity <= N) {
            for(u32 i = 0; i < size; i++) {
                if(tiny_data[i] == e) {
                    return true;
                }
            }
        } else {
            for(u32 i = 0; i < size; i++) {
                if(data[i] == e) {
                    return true;
                }
            }
        }
        return false;
    }

    /* Slice compatibility */

    Slice<T> full_slice() {
        if(capacity <= N) {
            return Slice<T> { .data = tiny_data, .size = size };
        } else {
            return Slice<T> { .data = data, .size = size };
        }
    }
    Slice<T> slice(u32 start, u32 size) {
        assert(start+size <= this->size);
        if(capacity <= N) {
            return Slice<T> { .data = tiny_data + start, .size = size };
        } else {
            return Slice<T> { .data = data + start, .size = size };
        }
    }

    /* STL Compatibility */

    T* begin() {
        if(capacity <= N) {
            return tiny_data;
        } else {
            return data;
        }
    }
    T* end() {
        if(capacity <= N) {
            return tiny_data + size;
        } else {
            return data + size;
        }
    }

    bool operator==(const TinyVec<T,N>& other) const {
        if(size != other.size) return false;
        if(capacity <= N) {
            for(u32 i = 0; i < size; i++) {
                if(tiny_data[i] != other[i]) return false;
            }
        } else {
            for(u32 i = 0; i < size; i++) {
                if(data[i] != other[i]) return false;
            }
        }
        return true;
    }
};

template <typename T, u32 N>
std::ostream& operator<<(std::ostream& os, TinyVec<T,N>& vec) {
    os << '[';
    for(T& e : vec) {
        os << e << ',';
    }
    os << ']';
    return os;
}