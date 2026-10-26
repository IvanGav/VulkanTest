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
    // return true if attacked (and false if did not attack)
    virtual u32 get_cooldown_ticks() const = 0;
    virtual bool attack(ntower::Tower* tower, nproj::Buff const& buff_context) const = 0;
};

struct Attack {
    Proto* proto;
    u32 attack_on_tick; // at/after this tick, can attack

    bool tick(ntower::Tower* tower, nproj::Buff const& buff_context) {
        if (data::update >= attack_on_tick) {
            if (proto->attack(tower, buff_context)) {
                attack_on_tick = data::update + proto->get_cooldown_ticks();
                return true;
            }
        }
        return false;
    }
};

}

