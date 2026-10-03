#pragma once

#include "../core/prelude.h"
#include "../core/mem.h"
#include "../core/slice.h"
#include "../core/str.h"
#include "../core/pair.h"
#include "../core/vec.h"
#include "../core/tinyvec.h"

//mem::Arena global_arena; // never cleared
//mem::Arena game_arena; // cleared when starting/exiting a game
//mem::Arena round_arena; // cleared at the end of every round
//mem::Arena frame_arena; // cleared every frame

u64 first_n_bits_mask(u8 number_of_bits_to_mask) {
    return number_of_bits_to_mask == 64 ? U64_MAX : (u64(1) << number_of_bits_to_mask) - 1;
}

struct Vec2 {
    f32 x;
    f32 y;

    bool operator==(const Vec2& other) const {
        return x == other.x && y == other.y;
    }

    Vec2 operator-(const Vec2& other) const {
        return Vec2 { .x = x - other.x, .y = y - other.y };
    }
    Vec2 operator+(const Vec2& other) const {
        return Vec2 { .x = x + other.x, .y = y + other.y };
    }

    f32 dir() {
        return atan2(y, x);
    }
    Vec2 abs() const {
        return Vec2 { .x = x > 0.0f ? x : -x, .y = y > 0.0f ? y : -y };
    }
    f32 len() const {
        return sqrt(x*x + y*y);
    }
    f32 len_sq() const {
        return x*x + y*y;
    }
    Vec2 rotate(f32 angle) const {
        f32 s = sin(angle), c = cos(angle);
        //std::sincosf32(angle, &sin, &cos); // TODO
        f32 len = this->len();
        return Vec2 { .x = len * c, .y = len * s };
    }
};

struct Vec3 {
    f32 x;
    f32 y;
    f32 z;

    bool operator==(const Vec3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};