#pragma once

#include "prelude.h"
#include "attack.h"

namespace ntower {

    struct Proto {
        Slice<nattack::Proto*> attacks;
        Slice<nattack::TargetMode> target_modes;
    };

    struct Tower {
        Proto* proto;
        Vec2 pos;
        f32 dir;
        Vec<nattack::Attack> attacks;
        u32 cur_target_mode;
        Vec<nproj::Buff> buffs;

        u32 attacked_on;

        static Tower spawn(Proto* proto, Vec2 pos) {
            Vec<nattack::Attack> attacks{ .arena = &game_arena };
            for (nattack::Proto* attack_proto : proto->attacks) {
                attacks.push({ .proto = attack_proto, .attack_on_tick = data::update });
            }
            return Tower{
                .proto = proto,
                .pos = pos,
                .dir = 0.0f, // assume initial rotation
                .attacks = attacks, // ok to (mem) copy since the original goes out of scope
                .cur_target_mode = 0,
                .buffs = Vec<nproj::Buff> { .arena = &game_arena },
                .attacked_on = 0,
            };
        }

        void tick() {
            nproj::Buff buff_context = {};
            for (nproj::Buff const& buff : buffs) {
                buff_context = buff_context + buff;
            }
            for (nattack::Attack& attack : attacks) {
                bool attacked = attack.attack(pos, dir, buff_context, proto->target_modes[cur_target_mode]);
                if (attacked) attacked_on = data::update;
            }
        }
    };

}