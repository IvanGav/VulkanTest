#pragma once

#include "prelude.h"
#include "plist.h"
#include "../graphics/data.h"

namespace nattack {

enum class TargetMode {
    FirstBloon, StrongBloon, LastBloon, CloseBloon,
    CloseRoad, FarRoad, SmartRoad,
    Always, InRange,
};

struct Proto {
    // return true if attacked (and false if did not attack)
    virtual u32 get_cooldown_ticks() const = 0;
    virtual bool attack(Vec2 tower_pos, f32& tower_dir, nproj::Buff const& buff_context, TargetMode mode) const = 0;
};

// directional attack spawning `count` projectiles with `spread` angle between each of them
struct PSimple : Proto {
    f32 range;
    u32 cooldown_ticks;
    u32 count;
    f32 spread;
    nproj::Proto* proj;
    PSimple(f32 range, u32 cooldown_ticks, u32 count, f32 spread, nproj::Proto* proj) : range(range), cooldown_ticks(cooldown_ticks), count(count), spread(spread), proj(proj) {};
    u32 get_cooldown_ticks() const override { return cooldown_ticks; };
    bool attack(Vec2 tower_pos, f32& tower_dir, nproj::Buff const& buff_context, TargetMode mode) const override {
        assert(mode == TargetMode::CloseBloon); // TODO
        nbloon::Bloon const* target = nullptr;
        f32 closest_dist = F32_INF;
        for (nbloon::Bloon const& bloon : bloons->bloons) {
            f32 dist = (bloon.pos - tower_pos).len();
            if (dist < closest_dist && dist <= range) {
                closest_dist = dist;
                target = &bloon;
            }
        }
        // fire
        if (target == nullptr) { return false; }
        tower_dir = (target->pos - tower_pos).dir();
        if (count == 1) {
            projs->add(nproj::Projectile::spawn(proj, buff_context, tower_pos, tower_dir));
        } else {
            for (u32 i = 0; i < count; i++) {
                f32 gap_angle = spread / (count - 1);
                f32 proj_dir = tower_dir - spread / 2 + i * gap_angle;
                projs->add(nproj::Projectile::spawn(proj, buff_context, tower_pos, proj_dir));
            }
        }
        return true;
    };
};

struct Attack {
    Proto* proto;
    u32 attack_on_tick; // at/after this tick, can attack

    bool attack(Vec2 tower_pos, f32& tower_dir, nproj::Buff const& buff_context, TargetMode mode) {
        if (data::update >= attack_on_tick) {
            if (proto->attack(tower_pos, tower_dir, buff_context, mode)) {
                attack_on_tick = data::update + proto->get_cooldown_ticks();
                return true;
            }
        }
        return false;
    }
};

}