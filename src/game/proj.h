#pragma once

#include "prelude.h"
#include "blist.h"

namespace nproj {

// careful; owned data
struct DamageBonus {
    TinyVec<P<nbloon::Type,u32>, 4> bonus;

    DamageBonus& operator+=(DamageBonus const& other) {
        // TODO implement this smarter, since i'm already sorting it anyway
        nbloon::Type included = {};
        for (u32 i = 0; i < bonus.size; i++) {
            bonus[i].b += other.bonus_damage_against(bonus[i].a);
            included = included | bonus[i].a;
        }
        for (u32 i = 0; i < other.bonus.size; i++) {
            if (!included.has_any(other.bonus[i].a)) {
                bonus.push(other.bonus[i]);
            }
        }
        std::sort(bonus.begin(), bonus.end());
        return *this;
    }

    u32 bonus_damage_against(nbloon::Type btype) const {
        for (u32 i = 0; i < bonus.size; i++) {
            if (bonus[i].a == btype) { return bonus[i].b; }
        }
        return 0;
    }
    auto operator<=>(DamageBonus const&) const = default;
};

typedef u8 Flags;

enum class FlagsBit {
    OverkillBlimp = 1, // if true, can overkill blimps
    SkipRecordHits = 2, // if true, will not record hit bloons (aka can hit same bloon every tick)
    CannotReceiveEffects = 4, // if true, buff effects will not apply
    CannotReceiveLifetimeEffects = 8, // if true, buff lifetime effects will not apply
    Unused16 = 16,
    Unused32 = 32,
    Unused64 = 64,
    Unused128 = 128,
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
    nbloon::EffectProto apply_effect;

    visual::Static sprite;
};

struct Effect {
    u32 damage;
    u32 pierce;
    nbloon::Type can_hit_and_pop_type;
    // for now can't modify movement/speed
    // can never modify hitbox; MERMONKEY MIDDLE PATH: EXISTS
    // for now can't modify lifetime
    // for now can't modify flags
    DamageBonus damage_bonus_add;

    // Get effect that, when added (operator+) to another EffectProto, does not change it
    static Effect unit() {
        return Effect{
            .damage = 0,
            .pierce = 0,
            .can_hit_and_pop_type = {},
            .damage_bonus_add = { .bonus = { .arena = &round_arena} },
        };
    }

    Effect& operator+=(Effect const& other) {
        damage += other.damage;
        pierce += other.pierce;
        can_hit_and_pop_type = can_hit_and_pop_type | other.can_hit_and_pop_type;
        damage_bonus_add += other.damage_bonus_add;
        return *this;
    }

    auto operator<=>(Effect const&) const = default;
};

// Projectile ID
// Bevy's "Entity" equivalent; Used to index into "Slot Map" `PList` (declared in `plist.h`)
//struct PID {
//    u32 i;
//    u32 gen;
//};

struct Projectile {
    Proto* proto;
    Effect effect;
    u32 hit_bloons_i; // index of the chunk in `plist::HitLedger`
    u32 pierce;
    nmove::Move movement;
    u32 lifetime_ticks;
    TID parent_tower;

    // these are what actually defines the projectile on the map;
    Vec2 pos;
    f32 dir;

    static Projectile spawn(Proto* proto, Effect effect, Vec2 pos, f32 dir, TID tower_id) {
        return nproj::Projectile{
            .proto = proto,
            .effect = effect,
            .pierce = proto->pierce + effect.pierce,
            .movement = proto->move,
            .lifetime_ticks = proto->lifetime_ticks,
            .parent_tower = tower_id,
            .pos = pos,
            .dir = dir
        };
    }

    void move() {
        lifetime_ticks -= 1;
        movement.move(pos, dir);
    }

    // return true if able to interact with this bloon type
    bool can_hit(nbloon::Type bt) const {
        // TODO apply buff
        return !proto->cannot_hit_type.has_any(bt);
    }

    // return true if able to pop this bloon type
    bool can_pop(nbloon::Type bt) const {
        // TODO apply buff
        return !proto->cannot_pop_type.has_any(bt);
    }

    bool can_overkill(nbloon::Type bt) const {
        return !(!has_flag(nproj::FlagsBit::OverkillBlimp) && bt.has_any(BTYPE_BLIMP));
    }

    bool has_flag(FlagsBit bit) const {
        return proto->flags & (Flags) bit;
    }

    u32 get_damage(nbloon::Type hit_bloon_type) const {
        return proto->damage + effect.damage + proto->damage_bonus_add.bonus_damage_against(hit_bloon_type) + effect.damage_bonus_add.bonus_damage_against(hit_bloon_type);
    }
};

}