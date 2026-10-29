#pragma once

#include "prelude.h"
#include "plist.h"

namespace ntower {

    // return true if attacked (don't have to attack when called because, for example, may have no targets in range)
    typedef bool(*attack_fn)(Tower*, nproj::Buff const&);
    typedef u32(*get_cooldown_ticks_fn)();

    struct Attack {
        /* prototype */
        std::function<bool(Tower*, nproj::Buff const&)>* attack;
        u32 cooldown_ticks;
        /* state */
        u32 attack_on_tick; // at/after this tick, can attack

        Attack instantiate() const {
            Attack clone = *this;
            clone.attack_on_tick = data::update; // may fire right away
            return clone;
        }

        bool tick(ntower::Tower* tower, nproj::Buff const& buff_context) {
            if (data::update >= attack_on_tick) {
                if ((*attack)(tower, buff_context)) {
                    attack_on_tick = data::update + cooldown_ticks;
                    return true;
                }
            }
            return false;
        }
    };

    enum class TargetMode {
        FirstBloon, StrongBloon, LastBloon, CloseBloon,
        CloseRoad, FarRoad, SmartRoad,
        Always, InRange,
    };

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
        Slice<Attack> attacks;
        Slice<TargetMode> target_modes;
    };

    struct Tower {
        // tower identity and state
        Proto* proto;
        Vec2 pos;
        f32 dir;
        Vec<Attack> attacks;
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
        bool camo_priority; // hardcoded, regretably
        u32 attacked_on;
        //AnimationStateMachine anim_state;

        static Tower spawn(Proto* proto, Vec2 pos) {
            Vec<Attack> attacks{ .arena = &game_arena };
            for (Attack& attack_proto : proto->attacks) {
                attacks.push(attack_proto.instantiate());
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

        TargetMode get_target_mode() const { return proto->target_modes[cur_target_mode]; }

        void tick() {
            nproj::Buff buff_context = {};
            for (nproj::Buff const& buff : buffs) {
                buff_context = buff_context + buff;
            }
            for (Attack& attack : attacks) {
                bool attacked = attack.tick(this, buff_context);
                if (attacked) attacked_on = data::update;
            }
        }
    };

}

// TODO do something about this
const ntower::Type TTYPE_DART_MONKEY = 0;