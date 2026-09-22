#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdalign.h>
#include <type_traits>
//#include <unistd.h>
#include <vector>

using std::move;

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef uintptr_t usize;
typedef float f32;
typedef double f64;

#define U8_MAX 0xFF
#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFFULL
#define I8_MAX 0x7F
#define I16_MAX 0x7FFF
#define I32_MAX 0x7FFFFFFF
#define I64_MAX 0x7FFFFFFFFFFFFFFFLL
#define I8_MIN i8(0x80)
#define I16_MIN i16(0x8000)
#define I32_MIN i32(0x80000000)
#define I64_MIN i64(0x8000000000000000LL)
#define F32_SMALL (__builtin_bit_cast(f32, 0x00800000u))
#define F32_LARGE (__builtin_bit_cast(f32, 0x7F7FFFFFu))
#define F32_INF (__builtin_bit_cast(f32, 0x7F800000u))
#define F32_QNAN (__builtin_bit_cast(f32, 0x7FFFFFFFu))
#define F32_SNAN (__builtin_bit_cast(f32, 0x7FBFFFFFu))
#define F64_SMALL (__builtin_bit_cast(f64, 0x0010000000000000ull))
#define F64_LARGE (__builtin_bit_cast(f64, 0x7FEFFFFFFFFFFFFFull))
#define F64_INF (__builtin_bit_cast(f64, 0x7FF0000000000000ull))
#define F64_QNAN (__builtin_bit_cast(f64, 0x7FFFFFFFFFFFFFFFull))
#define F64_SNAN (__builtin_bit_cast(f64, 0x7FF7FFFFFFFFFFFFull))

#define KB *1024
#define MB *1024*1024

#define unreachable { std::cout << ("UNREACHABLE REACHED\n"); assert(false); /*__builtin_unreachable();*/ std::abort(); }
#define panic { std::cout << ("PANIC\n"); assert(false); std::abort(); }
#define todo { std::cout << ("TODO\n"); assert(false); std::abort(); }
#define warn { std::cout << "WARN: " << __FILE__ << ":" << __LINE__ << "\n"; }

#define ceil_div(num, denom) (num/denom + (num%denom != 0))

#ifndef DONT_PRINTD
#define printd(expr) { std::cout << "--DEBUG " #expr ": " << (expr) << std::endl; }
#define logd(expr) { std::cout << "--DEBUG: " << expr << std::endl; }
#else
#define printd(expr) { }
#define logd(expr) { }
#endif

#define printe(message, expr) std::cout << "--ERROR " message ": " << (expr) << std::endl;

template <typename... Args>
void err(Args&&... strs) {
    std::cout << "ERROR: ";
    ((std::cout << std::forward<Args>(strs) << " "), ...);
    std::cout << std::endl;
    while(true) { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }
    exit(1);
}

template <typename T>
T max(T a, T b) { return a > b ? a : b; }

template <typename T>
T min(T a, T b) { return a < b ? a : b; }

usize next_power_of_two(usize n) {
    assert(sizeof(usize) == sizeof(unsigned long long));
    i32 leading_zeros = std::countl_zero(n);
    usize closest_pow_2 = (usize)1 << (sizeof(usize)*8 - leading_zeros - 1);
    if(n != closest_pow_2) closest_pow_2 <<= 1;
    return closest_pow_2;
}

static const usize prime_sizes[] = {
    // definitely primes:
    53, 97, 193, 389, 769,
    1543, 3079, 6151, 12289, 24593,
    49157, 98317, 196613, 393241,
    786433, 1572869, 3145739,
    // not tested for being primes:
    6291469, 12582917, 25165843,
    50331653, 100663319, 201326611,
    402653189, 805306457, 1610612741
};

usize next_prime_size(usize n) {
    for(usize i = 0; i < 26; i++) {
        if(prime_sizes[i] > n) return prime_sizes[i];
    }
    panic;
}

template <typename T>
T& ref(T&& rval) {
    return rval;
}