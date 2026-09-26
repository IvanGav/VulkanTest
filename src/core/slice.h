#pragma once

#include "prelude.h"
#include "mem.h"

//template <typename T> struct OwnedSlice;
template <typename T> struct Slice;

// Immutable slice on contiguous memory
template <typename T>
struct Slice {
    T* data; // nonull
    u32 size;

    static Slice<T> from(T* ptr, u32 size) { return Slice<T> {.data = ptr, .size = size }; }
    static Slice<T> from(Slice<T> slice) { return slice; }
    static Slice<u8> from(char const* ptr) { return Slice<u8> {.data = (u8*)ptr, .size = (u32)strlen(ptr) }; }
    static Slice<u8> from(char* ptr) { return Slice<u8> { .data = (u8*)ptr, .size = (u32)strlen(ptr) }; }
    static Slice<T> from(std::vector<T>& vec) { return Slice<T> {.data = vec.data(), .size = (u32)vec.size() }; }

    //OwnedSlice<T> to_owned() {
    //    return OwnedSlice<T>::from(*this);
    //}

    T const& operator[](u32 i) const {
        assert(i >= 0);
        assert(i < size);
        return data[i];
    }

    bool operator==(const Slice& other) const {
        if (size != other.size) return false;
        for (u32 i = 0; i < size; ++i) {
            if (!(data[i] == other.data[i])) return false;
        }
        return true;
    }

    Slice<T> slice(u32 start, u32 size) {
        assert(start+size <= this->size);
        return Slice<T> { .data = data + start, .size = size };
    }
    // `end` exclusive
    Slice<T> slice_range(u32 start, u32 end) {
        assert(end <= this->size);
        return Slice<T> { .data = data + start, .size = end - start };
    }

    /* STL Compatibility */

    T* begin() {
        return data;
    }
    T* end() {
        return data + size;
    }
    //u64 hash() {
    //    u64 acc_hash = 0;
    //    for(u32 i = 0; i < size; i++) {
    //        acc_hash ^= std::rotl(hash::from(data[i]), i);
    //    }
    //    return acc_hash;
    //}
};

// Owned/mutable slice on contiguous memory. Frees memory when goes out of scope.
//template <typename T>
//struct OwnedSlice {
//    T* data; // nonull
//    u32 size;
//
//    static OwnedSlice<T> from_unsafe(T* ptr, u32 size) {
//        return OwnedSlice<T> { .data = ptr, .size = size };
//    }
//    static OwnedSlice<T> create(u32 size) {
//        return OwnedSlice<T> { .data = mem::alloc<T>(size), .size = size };
//    }
//    static OwnedSlice<u8> from(Slice<u8> str) {
//        return OwnedSlice<u8> { .data = (u8*) mem::clone(str.data, str.size), .size = str.size };
//    }
//    static OwnedSlice<u8> from(std::string& str) {
//        return OwnedSlice<u8> { .data = (u8*) mem::clone(str.data(), str.size()), .size = str.size() };
//    }
//
//    ~OwnedSlice() {
//        mem::free(data);
//    }
//
//    T& operator[](u32 i) const {
//        assert(i >= 0);
//        assert(i < size);
//        return data[i];
//    }
//
//    bool operator==(const OwnedSlice<T>& other) const {
//        if (size != other.size) return false;
//        for (u32 i = 0; i < size; ++i) {
//            if (!(data[i] == other.data[i])) return false;
//        }
//        return true;
//    }
//    bool operator==(const Slice<T>& other) const {
//        if (size != other.size) return false;
//        for (u32 i = 0; i < size; ++i) {
//            if (!(data[i] == other.data[i])) return false;
//        }
//        return true;
//    }
//
//    Slice<T> slice() {
//        return Slice<T> { .data = data, .size = size };
//    }
//    Slice<T> slice(u32 start, u32 size) {
//        assert(start+size <= this->size);
//        return Slice<T> { .data = data + start, .size = size };
//    }
//    // `end` exclusive
//    Slice<T> slice_range(u32 start, u32 end) {
//        assert(end <= this->size);
//        return Slice<T> { .data = data + start, .size = end - start };
//    }
//
//    // Special overrides to disallow copying and enable `std::move`, for safety
//    //OwnedSlice() {}
//    //OwnedSlice(const OwnedSlice&) = delete;
//    //OwnedSlice& operator=(const OwnedSlice&) = delete;
//    //OwnedSlice(OwnedSlice&& other) noexcept {
//    //    data = other.data;
//    //    size = other.size;
//    //    other.data = nullptr;
//    //    other.size = 0;
//    //}
//    //OwnedSlice& operator=(OwnedSlice&& other) {
//    //    if (this != &other) {
//    //        mem::free(data);
//    //        data = other.data;
//    //        size = other.size;
//    //        other.data = nullptr;
//    //        other.size = 0;
//    //    }
//    //    return *this;
//    //}
//
//    // Shallow clone
//    OwnedSlice<T> clone() {
//        return OwnedSlice { .data = mem::clone(data, size), .size = size };
//    }
//
//    /* STL Compatibility */
//
//    T* begin() {
//        return data;
//    }
//    T* end() {
//        return data + size;
//    }
//};