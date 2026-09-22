#pragma once

#include "mem.h"
#include "slice.h"
#include "vec.h"

//typedef OwnedSlice<u8> OwnedStr;
typedef Slice<u8> Str;

namespace str {
    Str from_slice_of_str(mem::Arena* arena, Slice<Str>& v) {
        u32 comb_size = 0;
        for (Str& s : v) comb_size += s.size;
        Str combined = Str{ .data = arena->alloc<u8>(comb_size), .size = comb_size };

        u32 accumulated = 0;
        for (Str& s : v) {
            mem::copy(s.data, combined.data + accumulated, s.size);
            accumulated += s.size;
        }
        return combined;
    }

    template <typename... Args>
    Str cat(mem::Arena* arena, Args&&... strs) {
        mem::Arena* scratch = get_scratch();
        Str concat;
        arena_scope(scratch) {
            Vec<Str> vec = {.arena = scratch};
            (vec.push(Str::from(std::forward<Args>(strs))), ...);
            concat = str::from_slice_of_str(arena, ref(vec.full_slice()));
        }
        return concat;
    }

    template <typename... Args>
    Str cat_global(Args&&... strs) {
        return str::cat(&global_arena, std::forward<Args>(strs)...);
    }
}

Str operator ""s(const char* s, size_t len) {
    return Str::from((u8*) s, (u32)len);
}
//OwnedStr operator ""os(const char* s, size_t len) {
//    return OwnedStr::from(Str::from((u8*) s, len));
//}
std::ostream& operator<<(std::ostream& os, const Str& str) {
    os.write((const char*) str.data, (std::streamsize) str.size);
    return os;
}
//std::ostream& operator<<(std::ostream& os, const OwnedStr& str) {
//    os.write((const char*) str.data, (std::streamsize) str.size);
//    return os;
//}