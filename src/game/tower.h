#pragma once

#include "prelude.h"
#include "attack.h"

namespace ntower {

    // TODO I can afford to make it a bit mask, but if need be, change to 0-4 later
    enum class Category {
        Primary = 1,
        Military = 2,
        Magic = 4,
        Support = 8,
        Hero = 16
    };

    typedef u32 Type;

    struct Proto {
        Category category;
        Type type;
        Slice<nattack::Proto*> attacks;
        Slice<nattack::TargetMode> target_modes;
    };

    struct Tower {
        // tower identity and state
        Proto* proto;
        Vec2 pos;
        f32 dir;
        Vec<nattack::Attack> attacks;
        u32 cur_target_mode;
        Vec<nproj::Buff> buffs;

        // parent-child definitions
        TID id;
        TID parent;
        Vec<TID> children;

        // statistics
        u32 pop_count;
        u32 bonus_money_count;

        // animation info
        u32 attacked_on;
        //AnimationStateMachine anim_state;

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

        nattack::TargetMode get_target_mode() const { return proto->target_modes[cur_target_mode]; }

        void tick() {
            nproj::Buff buff_context = {};
            for (nproj::Buff const& buff : buffs) {
                buff_context = buff_context + buff;
            }
            for (nattack::Attack& attack : attacks) {
                bool attacked = attack.tick(this, buff_context);
                if (attacked) attacked_on = data::update;
            }
        }
    };

}



// TODO move from here

namespace nattack {


    /* Specialized Prototypes */

    // directional attack spawning `count` projectiles with `spread` angle between each of them
    struct PSimple : Proto {
        f32 range;
        u32 cooldown_ticks;
        u32 count;
        f32 spread;
        nproj::Proto* proj;
        PSimple(f32 range, u32 cooldown_ticks, u32 count, f32 spread, nproj::Proto* proj) : range(range), cooldown_ticks(cooldown_ticks), count(count), spread(spread), proj(proj) {};
        u32 get_cooldown_ticks() const override { return cooldown_ticks; };
        bool attack(ntower::Tower* tower, nproj::Buff const& buff_context) const override {
            assert(tower->get_target_mode() == TargetMode::CloseBloon); // TODO
            nbloon::Bloon const* target = nullptr;
            f32 closest_dist = F32_INF;
            for (nbloon::Bloon const& bloon : bloons->list) {
                f32 dist = (bloon.pos - tower->pos).len();
                if (dist < closest_dist && dist <= range) {
                    closest_dist = dist;
                    target = &bloon;
                }
            }
            // fire
            if (target == nullptr) { return false; }
            tower->dir = (target->pos - tower->pos).dir();
            if (count == 1) {
                projs->add(nproj::Projectile::spawn(proj, buff_context, tower->pos, tower->dir));
            }
            else {
                for (u32 i = 0; i < count; i++) {
                    f32 gap_angle = spread / (count - 1);
                    f32 proj_dir = tower->dir - spread / 2 + i * gap_angle;
                    projs->add(nproj::Projectile::spawn(proj, buff_context, tower->pos, proj_dir));
                }
            }
            return true;
        };
    };
}