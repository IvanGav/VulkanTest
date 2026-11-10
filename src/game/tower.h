#pragma once

#include "prelude.h"
#include "plist.h"

namespace ntower {

    // When effect with same `id` is applied, it simply replaces the previous instance of this effect `id`
    // For now, there isn't a way to do same effect stacking TODO
    // For now, there isn't a way to replace "weaker" effect with "stronger"
    struct EffectProto {
        const char* name;
        u32 id;

        u32 duration_ticks;
        u32 max_stacks;

        nproj::Effect proj_effect;

        f32 attack_cooldown_mult;

        // Get effect that, when added (operator+) to another EffectProto, does not change it
        static EffectProto unit() {
            return EffectProto{
                .proj_effect = nproj::Effect::unit(),
                .attack_cooldown_mult = 1.0f
            };
        }
        bool is_anonymous() const { return name == nullptr; }

        EffectProto& operator+=(EffectProto const& other) {
            proj_effect += other.proj_effect;
            attack_cooldown_mult *= other.attack_cooldown_mult;
            return *this;
        }
        auto operator<=>(EffectProto const&) const = default;
    };

    struct Effect {
        EffectProto* proto;
        u32 expire_tick;
        u32 stacks;

        static Effect spawn(EffectProto* proto) {
            return Effect{
                .proto = proto,
                .expire_tick = proto->duration_ticks == U32_MAX ? U32_MAX : data::update + proto->duration_ticks
                // TODO upgrade to C++26 for this:
                //.expire_tick = std::saturating_add(data::update, proto->duration_ticks)
            };
        }

        bool is_expired() const { return data::update >= expire_tick; }
    };

    // return true if attacked (don't have to attack when called because, for example, may have no targets in range)
    typedef bool(*attack_fn)(Tower*, nproj::Effect const&);

    struct Attack {
        /* prototype */
        std::function<bool(Tower*, nproj::Effect const&)>* attack;
        u32 cooldown_ticks;
        /* state */
        u32 attack_on_tick; // at/after this tick, can attack

        Attack instantiate() const {
            Attack clone = *this;
            clone.attack_on_tick = data::update; // may fire right away
            return clone;
        }

        bool tick(ntower::Tower* tower, EffectProto const& buff_context) {
            if (data::update >= attack_on_tick) {
                if ((*attack)(tower, buff_context.proj_effect)) {
                    attack_on_tick = data::update + u32(roundf(cooldown_ticks * buff_context.attack_cooldown_mult));
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
        visual::Animated sprite;
        const char* tower_name;
    };

    struct Tower {
        // tower identity and state
        Proto* proto;
        Vec2 pos;
        f32 dir;
        Vec<Attack> attacks;
        u32 cur_target_mode;
        TinyVec<Effect, 4> effects;

        // parent-child definitions
        TID id;
        TID parent;
        Vec<TID> children;

        // statistics
        u32 pop_count;
        u32 bonus_money_count;

        // animation info
        bool camo_priority; // hardcoded, regretably
        visual::Animated sprite;

        static Tower spawn(Proto* proto, Vec2 pos, f32 dir) {
            Vec<Attack> attacks{ .arena = &game_arena };
            for (Attack& attack_proto : proto->attacks) {
                attacks.push(attack_proto.instantiate());
            }
            return Tower{
                .proto = proto,
                .pos = pos,
                .dir = dir,
                .attacks = attacks, // ok to (mem) copy since the original goes out of scope
                .cur_target_mode = 0,
                .effects = TinyVec<Effect, 4> {.arena = &game_arena },
                .sprite = proto->sprite.instanciate()
            };
        }

        TargetMode get_target_mode() const { return proto->target_modes[cur_target_mode]; }

        void tick() {
            EffectProto buff_context = EffectProto::unit();
            for (Effect const& effect : effects) {
                buff_context += *effect.proto;
            }
            for (Attack& attack : attacks) {
                bool attacked = attack.tick(this, buff_context);
                if (attacked) { sprite.transition(visual::AnimationState::Attack); }
            }
            sprite.do_auto_transition();
        }
    };

}

// TODO do something about this
const ntower::Type TTYPE_DART_MONKEY = 0;