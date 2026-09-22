#pragma once

#include "prelude.h"
#include "mem.h"

// any methods that accept size/capacity, will accept number of bits, not bytes or words
struct BitSet {
    typedef u8 bits;
    bits* data; // nullable, owned
    u32 size; // real size of the array (not in bits)

    mem::Arena* arena;

    // new_size = in sizeof(bits), not bits
    void reserve(u32 new_size) {
        data = arena->realloc(data, size, new_size);
        mem::zero(data+size, new_size-size); // zero out the new bytes
        size = new_size;
    }

    // num = number of bits to reserve for
    void reserve_bits(u32 num) {
        u32 i = ceil_div(num,(sizeof(bits)*8));
        if(i >= size) { this->reserve(i); }
    }

    void set(u32 num) {
        u32 i = num/(sizeof(bits)*8);
        u32 offset = num%(sizeof(bits)*8);
        if(i >= size) { this->reserve(next_power_of_two(i+1)); }
        data[i] |= (bits(1) << offset);
    }

    void unset(u32 num) {
        u32 i = num/(sizeof(bits)*8);
        u32 offset = num%(sizeof(bits)*8);
        if(i >= size) { return; }
        data[i] &= bits(~(bits(1) << offset));
    }

    void toggle(u32 num) {
        if((*this)[num] == false) { this->set(num); }
        else { this->unset(num); }
    }

    // may be extremely expensive; return U32_MAX if none are set
    u32 next_set_bit(u32 start) {
        u32 i = start/(sizeof(bits)*8);
        if(data[i] >> (start%8) == 0) i++;
        for(; i < size; i++) {
            if(data[i] != 0) {
                u32 from = max(start, (u32) (i*sizeof(bits)*8)) % 8;
                for(u32 j = from; j < sizeof(bits)*8; j++) {
                    u32 bi = i*sizeof(bits)*8 + j;
                    if((*this)[bi]) return bi;
                }
            }
        }
        return U32_MAX;
    }

    /* Access Member Functions */

    bool operator[](u32 i) const {
        u32 bits_i = i/(sizeof(bits)*8);
        u32 bits_offset = i%(sizeof(bits)*8);
        if(bits_i >= size) { return false; }
        return (data[bits_i] >> bits_offset) & 1;
    }

    /* Util functions */

    void clear() {
        // size = 0;
        if(data == nullptr) return;
        mem::zero(data, size);
    }

    /* Cloning */

    // if no arena specified, copy to `this->arena`
    BitSet clone(mem::Arena* arena = nullptr) {
        if(arena == nullptr) arena = this->arena;
        BitSet newbs {
            .data = arena->alloc<bits>(size),
            .size = size,
        };
        mem::copy<bits>(newbs.data, data, size);
        return newbs;
    }
};

// potentially very expensive
std::ostream& operator<<(std::ostream& os, BitSet const& bitset) {
    os << '[';
    for(u32 i = 0; i < bitset.size * sizeof(BitSet::bits) * 8; i++) {
        os.put(bitset[i] ? '1' : '0');
    }
    os << ']';
    return os;
}