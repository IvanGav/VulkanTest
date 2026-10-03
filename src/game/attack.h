#pragma once

#include "prelude.h"
#include "plist.h"

namespace nattack {

enum class TargetMode {
    FirstBloon, StrongBloon, LastBloon, CloseBloon,
    CloseRoad, FarRoad, SmartRoad,
    Always, InRange,
};

struct Proto {
    virtual void attack(Vec2 tower_pos, nproj::Buff buff_context, TargetMode mode) const = 0;
};

// directional attack spawning `count` projectiles with `spread` angle between each of them
struct PSimple : Proto {
    f32 range;
    u32 cooldown_ticks;
    u32 count;
    f32 spread;
    nproj::Proto* proj;
    void attack(Vec2 tower_pos, nproj::Buff buff_context, TargetMode mode) const override { todo; };
};

struct Attack {
    Proto* proto;
    u32 attack_on_tick; // at/after this tick, can attack

    void attack(Vec2 tower_pos, nproj::Buff buff_context, TargetMode mode) { todo; }
};

}