#pragma once

#include "prelude.h"
#include "blist.h"

namespace proj {

// careful; owned data
struct DamageBonus {
    TinyVec<P<bloon::Type,u32>, 4> bonus;
};

enum class FlagsBit {
    OverkillBlimp = 1, // if true, can overkill blimps
    CannotReceiveEffects = 2, // if true, inherited effects will not apply
    CannotReceiveLifetimeEffects = 4, // if true, inherited lifetime effects will not apply
};

struct Flags {
    u8 flags;
};

struct Proto {
    u32 damage;
    u32 pierce;
    bloon::Type cannot_hit_type;
    bloon::Type cannot_pop_type;
    move::Move move;
    hitbox::HB hitbox;
    u32 lifetime_ticks;
    Flags flags;

    Proto* on_death_spawn_projectile;
    DamageBonus damage_bonus_add;
    // Slice<bloon::Effect> apply_effects;
};

struct Buff {
    u32 damage;
    u32 pierce;
    bloon::Type can_hit_and_pop_type;
    // for now can't modify movement/speed
    // can never modify hitbox
    // for now can't modify lifetime
    // for now can't modify flags
    DamageBonus damage_bonus_add;
};

struct Projectile {
    Proto* proto;
    Buff buff;
    u32 hit_bloons_i; // index of the chunk in `plist::HitLedger`

    // these are what actually defines the projectile on the map;
    hitbox::HB hb;
    Vec2 pos;
    f32 dir;
};

}