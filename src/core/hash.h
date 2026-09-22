#pragma once

#include "prelude.h"

// Does not support unions
namespace hash {
    template <std::integral T>
    u64 from(T i) {
        return (u64) i;
    }

    u64 from(f32 f) {
        return (u64) std::bit_cast<f32>(f);
    }

    u64 from(f64 f) {
        return (u64) std::bit_cast<f64>(f);
    }
    
    template <typename T>
    concept EnumClassConcept = std::is_enum_v<T> && !std::is_convertible_v<T, std::underlying_type_t<T>>; // I have no idea either

    template <EnumClassConcept T>
    u64 from(T e) {
        return hash::from((usize) e);
    }

    template <typename T>
    concept PointerConcept = std::is_pointer_v<T>;

    template <PointerConcept T>
    u64 from(T s) {
        // return s->hash();
        return (u64)(s); // simply cast the pointer to an int
    }

    template <typename T>
    u64 from(T s) {
        return s.hash();
    }
}