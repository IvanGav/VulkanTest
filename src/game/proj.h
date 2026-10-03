#pragma once

#include "prelude.h"
#include "blist.h"

namespace nproj {

// careful; owned data
struct DamageBonus {
    TinyVec<P<nbloon::Type,u32>, 4> bonus;
};

enum class FlagsBit {
    OverkillBlimp = 1, // if true, can overkill blimps
    CannotReceiveEffects = 2, // if true, buff effects will not apply
    CannotReceiveLifetimeEffects = 4, // if true, buff lifetime effects will not apply
};

struct Flags {
    u8 flags;
};

struct Proto {
    u32 damage;
    u32 pierce;
    nbloon::Type cannot_hit_type;
    nbloon::Type cannot_pop_type;
    nmove::Move move;
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
    nbloon::Type can_hit_and_pop_type;
    // for now can't modify movement/speed
    // can never modify hitbox
    // for now can't modify lifetime
    // for now can't modify flags
    DamageBonus damage_bonus_add;
};

// Projectile ID
// Bevy's "Entity" equivalent; Used to index into "Slot Map" `PList` (declared in `plist.h`)
//struct PID {
//    u32 i;
//    u32 gen;
//};

struct Projectile {
    //PID pid;
    Proto* proto;
    Buff buff;
    u32 hit_bloons_i; // index of the chunk in `plist::HitLedger`
    u32 pierce;
    nmove::Move movement;
    u32 lifetime_ticks;

    // these are what actually defines the projectile on the map;
    Vec2 pos;
    f32 dir;

    static Projectile spawn(Proto* proto, Buff& buff, Vec2 pos, f32 dir) {
        return nproj::Projectile{
            .proto = proto,
            .buff = buff,
            .pierce = proto->pierce,
            .movement = proto->move,
            .lifetime_ticks = proto->lifetime_ticks,
            .pos = pos,
            .dir = dir
        };
    }

    void move() {
        lifetime_ticks -= 1;
        movement.move(pos, dir);
    }
};

}