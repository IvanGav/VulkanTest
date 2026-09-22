#pragma once

#include "prelude.h"

namespace mem {
    // Allocate array of `size` elements of type `T`
    template <typename T>
    T* alloc(usize size) {
        return (T*) ::malloc(sizeof(T) * size);
    };

    // Free a given pointer
    template <typename T>
    void free(T* ptr) {
        assert(ptr != nullptr);
        ::free(ptr);
    };
    
    // Reallocate a given pointer (see `::realloc`)
    template <typename T>
    T* realloc(T* ptr, usize new_size) {
        return (T*) ::realloc((void*) ptr, sizeof(T) * new_size);
    };

    // Allocate array of `size` elements of type `T`
    template <typename T>
    T* make(T e) {
        T* ptr = mem::alloc<T>(1);
        *ptr = move(e);
        return ptr;
    };

    // Copy `size` elements of type `T` from `from` to `to`
    template <typename T>
    void copy(T const* from, T* to, usize size) {
        assert(from != nullptr);
        assert(to != nullptr);
        ::memcpy(to, from, sizeof(T) * size);
    }

    // Clone `size` elements of type `T` from `ptr` into newly allocated memory and return a pointer to it
    template <typename T>
    T* clone(T const* ptr, usize size) {
        assert(ptr != nullptr);
        T* cloned = mem::alloc<T>(size);
        mem::copy(ptr, cloned, size);
        return cloned;
    }

    // Zero out `size` elements of type `T`
    template <typename T>
    void zero(T* ptr, usize size) {
        assert(ptr != nullptr);
        ::memset((void*) ptr, 0, sizeof(T) * size);
    }

    // Given two pointers, swap memory behind those pointers
    template <typename T>
    void swap_arrays(T* l1, T* l2, usize size) {
        T* temp = mem::alloc<T>(size);
        mem::copy(&temp, l1, size);
        mem::copy(l1, l2, size);
        mem::copy(l2, &temp, size);
        mem::free(temp);
    }

    // Swap two elements
    template <typename T>
    void swap(T& l1, T& l2) {
        T temp;
        temp = l1;
        l1 = l2;
        l2 = temp;
    }

    /// Arena handles must be stored on a stack OR freed manually
    struct Arena {
        u8* data;
        u8* cur;
        u8* end_ptr;

        ~Arena() {
            mem::free(data);
        }
        
        void reset() {
            cur = data;
        }

        static Arena create(usize size) {
            u8* ptr = mem::alloc<u8>(size);
            return Arena { .data = ptr, .cur = ptr, .end_ptr = ptr+size };
        }

        // does **not** zero initialize
        template <typename T>
        T* alloc(usize size) {
            void* ptr = cur;
            usize size_left = end_ptr-cur;
            if (std::align(alignof(T), sizeof(T) * size, ptr, size_left) == nullptr) {
                std::cout << "Arena could not allocate memory" << std::endl;
                panic;
            }
            cur = (u8*) ptr + sizeof(T) * size;
            return (T*) ptr;
        }
        template <typename T>
        T* push(T item) {
            T* ptr = this->alloc<T>(1);
            *ptr = move(item);
            return ptr;
        }
        // does **not** zero initialize
        template <typename T>
        T* realloc(T* ptr, usize last_size, usize new_size) {
            if(last_size >= new_size) {
                return ptr;
            }
            if(((u8*) ptr + sizeof(T)*last_size) == cur) {
                // no new allocations were made
                cur += (new_size - last_size) * sizeof(T);
                return ptr;
            }
            // reallocate and copy memory
            T* out = this->alloc<T>(new_size);
            if(ptr != nullptr) mem::copy(ptr, out, last_size);
            return out;
        }

        template <typename T>
        T* clone(T const* ptr, usize size) {
            T* cloned = this->alloc<T>(size);
            mem::copy(ptr, cloned, size);
            return cloned;
        }
    };
}

const u32 SCRATCH_ARENA_NUM = 2;
u32 cur_scratch_arena = 0;
mem::Arena global_arena = mem::Arena::create(10 MB);
mem::Arena scratch_arenas[SCRATCH_ARENA_NUM] = { mem::Arena::create(10 MB), mem::Arena::create(10 MB) };

mem::Arena* get_scratch() {
    cur_scratch_arena = (cur_scratch_arena + 1) % SCRATCH_ARENA_NUM;
    return &scratch_arenas[cur_scratch_arena];
}

void drop_scratch() {
    cur_scratch_arena = (cur_scratch_arena - 1 + SCRATCH_ARENA_NUM) % SCRATCH_ARENA_NUM;
}

// after exiting this scope, reset the arena to its initial position at entrance
#define arena_scope(arena) for(u8* __ARENA_AT = arena->cur; __ARENA_AT; arena->cur = __ARENA_AT, __ARENA_AT = nullptr)
