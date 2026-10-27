#pragma once

#include "../core/prelude.h"
#include "../core/mem.h"
#include "../core/slice.h"
#include "../core/str.h"
#include "../core/pair.h"
#include "../core/vec.h"
#include "../core/tinyvec.h"
#include "../graphics/data.h"

namespace nbloon { struct Proto; struct Bloon; }
namespace nproj { struct Proto; struct Projectile; }
namespace ntower { struct Proto; struct Tower; }

namespace blist { struct BList; }
namespace plist { struct PList; }
namespace tlist { struct TList; }

// Generational ID - Bevy's "Entity" equivalent
// Null ID is just {.i=0,.gen=0}
// TODO ensure that this is still POD
template <typename T>
struct GID {
    u32 i;
    u32 gen;
    auto operator<=>(GID<T> const&) const = default;
};

// Specializations for the Generational IDs
typedef GID<nbloon::Bloon> BID;
typedef GID<nproj::Projectile> PID;
typedef GID<ntower::Tower> TID;

constexpr f32 range_btd_to_world(f32 btd_range) {
    return btd_range / 4; // TODO arbitrary
}

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