#pragma once

#include "prelude.h"
#include "hitbox.h"
#include "move.h"
#include "global.h"
#include "visual.h"

namespace nbloon {
// bitmask for lead/purple/white/black/etc
struct Type {
    typedef u16 TypeU;
    TypeU val;

    Type operator|(Type other) const { return Type{ .val = (TypeU)(this->val | other.val) }; }
    Type operator&(Type other) const { return Type{ .val = (TypeU)(this->val & other.val) }; }
    Type operator-(Type other) const { return Type{ .val = (TypeU)(this->val & ~other.val) }; } // bitclear
    bool has_each(Type other) const { return (val & other.val) == other.val; }
    bool has_any(Type other) const { return (val & other.val) != (TypeU)(0); }
    auto operator<=>(Type const&) const = default;
};
}

// TODO do something about this
const nbloon::Type BTYPE_BLIMP = { 1 << 0 };
const nbloon::Type BTYPE_CAMO = { 1 << 1 };
const nbloon::Type BTYPE_LEAD = { 1 << 2 };
const nbloon::Type BTYPE_CERAMIC = { 1 << 3 };
const nbloon::Type BTYPE_FROZEN = { 1 << 4 };

const nbloon::Type BTYPE_BLACK = { 1 << 5 };
const nbloon::Type BTYPE_WHITE = { 1 << 6 };
const nbloon::Type BTYPE_PURPLE = { 1 << 7 };
const nbloon::Type BTYPE_FORTIFIED = { 1 << 8 };

const nbloon::Type PTYPE_SHARP = BTYPE_LEAD | BTYPE_FROZEN;

namespace nbloon {

const f32 STEPS_CHILD_MOVE = 0.5f;

struct Proto {
    i32 hp;
    Type type;
    f32 speed;
    Slice<Proto*> children;
    hitbox::HB hitbox;
    f32 max_hitbox_dist; // distance to farthest point on a hitbox; basically defines the AABB of this hitbox
    bool speed_status_immune; // TODO probably replace with "f32 speed_status_mult" or something, because BFBs also get slowed down less... i think
    visual::Static sprite;
    const char* name;
    Type inherit_types; // types to be gained only if parent has them
};

// Bloon Family Tree
// Specifies parent-child relationship between bloons
u32 family_count = 0;
struct BFT {
    u32 tree;
    u32 family;
    u8 layer;

    static BFT spawn() {
        family_count++;
        return BFT { .tree = 0, .family = family_count, .layer = 0 };
    }
    static void reset_family_count() { family_count = 0; }
    static BFT null() { return BFT {.family = 0}; }
    bool is_null() const { return family == 0; }
    
    // Return true iff `self` is a parent or a child of `other`. That means, if self and `other` are in the same subtree.
    bool same_subtree_as(BFT other) const {
        if(family != other.family) { return false; }
        u8 min_layer = min(layer, other.layer);
        u32 mask = u32(first_n_bits_mask(min_layer));
        return (tree & mask) == (other.tree & mask);
    }

    // Given that this FT will produce `child_count` children total, get `child_num` child's FT
    // `child_num` is 0-indexed
    BFT get_child(u8 child_count, u8 child_num) const {
        assert(child_num < child_count);
        BFT child = *this; // clone self
        if(child_count == 1) {
            return child;
        }
        u8 add_layer = std::bit_width<u8>(child_count - 1);
        child.tree |= ((u32) child_num) << child.layer;
        child.layer += add_layer;
        return child;
    }

    bool operator==(const BFT& other) const {
        return family == other.family && tree == other.tree && layer == other.layer;
    }
};

typedef u8 Flags;

enum class FlagsBit {
    CannotSoakFromBlimp = 1 << 0, // if parent is blimp, do not soak
    CannotSoakToBlimp = 1 << 1, // if child is blimp, do not soak
    Unused4 = 1 << 2,
    Unused8 = 1 << 3,
    Unused16 = 1 << 4,
    Unused32 = 1 << 5,
    Unused64 = 1 << 6,
    Unused128 = 1 << 7
};

// When effect with same `id` is applied, it simply replaces the previous instance of this effect `id`
// For now, don't allow same effect stacking
// For now, there isn't a way to replace "weaker" effect with "stronger" (ie 300glue with 400glue) TODO
struct EffectProto {
    const char* name;
    u32 id;

    u32 duration_ticks;
    u32 soak_layer_count;

    f32 speed_mult;
    Type add_type;
    Type remove_type;
    f32 money_mult;
    i32 flat_damage_increase;
    Flags flags;

    // Damage Over Time
    i32 dot_damage;
    u32 dot_cooldown_ticks;

    // TODO on bloon death callback?

    // Get effect that, when added (operator+) to another EffectProto, does not change it
    static EffectProto unit() {
        return EffectProto{
            .speed_mult = 1.0f,
            .add_type = {},
            .remove_type = {},
            .money_mult = 1.0f,
            .flat_damage_increase = 0,
            .flags = 0
        };
    }
    bool is_anonymous() const { return name == nullptr; }
    bool is_unit() const { return *this == EffectProto::unit(); }

    // combine effect fields/modifiers that can be combined (ie speed, type, money, damage)
    // erase any other fields/modifiers (ie name, id, duration and DOT)
    EffectProto operator+(EffectProto const& other) const {
        return EffectProto{
            .speed_mult = speed_mult * other.speed_mult,
            .add_type = add_type | other.add_type,
            .remove_type = remove_type | other.remove_type,
            .money_mult = money_mult * other.money_mult,
            .flat_damage_increase = flat_damage_increase + other.flat_damage_increase,
            .flags = Flags(flags | other.flags)
        };
    }
    auto operator<=>(EffectProto const&) const = default;
};

struct Effect {
    EffectProto* proto;
    u32 expire_tick;
    u32 dot_activation_tick; // update on which dot should apply and reset this field
    u32 soak_layers_left;

    static Effect spawn(EffectProto* proto) {
        return Effect{
            .proto = proto,
            .expire_tick = proto->duration_ticks == U32_MAX ? U32_MAX : data::update + proto->duration_ticks,
            .dot_activation_tick = proto->dot_cooldown_ticks == U32_MAX ? U32_MAX : data::update + proto->dot_cooldown_ticks
            // TODO upgrade to C++26 for these:
            //.expire_tick = std::saturating_add(data::update, proto->duration_ticks),
            //.dot_activation_tick = std::saturating_add(data::update, proto->dot_cooldown_ticks)
        };
    }

    Effect pass_to_child() const {
        assert(soak_layers_left > 0);
        Effect e = *this;
        e.soak_layers_left -= 1;
        return e; // TODO optionally refresh the DOT
    }
    bool can_pass_to_child(Type self_type, Type child_type) const {
        if ((proto->flags & Flags(FlagsBit::CannotSoakFromBlimp)) != Flags(0) && self_type.has_any(BTYPE_BLIMP)) { return false; }
        if ((proto->flags & Flags(FlagsBit::CannotSoakToBlimp)) != Flags(0) && child_type.has_any(BTYPE_BLIMP)) { return false; }
        return soak_layers_left > 0;
    }
    bool is_expired() const { return data::update >= expire_tick; }
    bool is_dot_ready() const { return data::update >= dot_activation_tick; }
    void refresh_dot() { assert(proto->dot_cooldown_ticks != U32_MAX); dot_activation_tick = data::update + proto->dot_cooldown_ticks; }
};

// represents a Bloon entity
struct Bloon {
    Vec2 pos;
    BID id;
    BFT bft;
    Type type; // type can sometimes change dynamically
    f32 dir;
    i32 hp; // current hp
    nmove::MoveAlongRoad movement;
    Proto* proto;
    TinyVec<Effect, 4> effects;
    EffectProto cached_cumulative_effect;

    static Bloon spawn(Proto* proto, npath::Path* path) {
        return Bloon {
            .pos = path->nodes[0],
            .bft = BFT::spawn(),
            .type = proto->type,
            .dir = (path->nodes[1] - path->nodes[0]).dir(),
            .hp = (i32) proto->hp,
            .movement = nmove::MoveAlongRoad { .waypoint = path->nodes[1], .target_node = 1, .road_pos = 0.0, .speed = proto->speed, .path = path },
            .proto = proto,
            .effects = { .arena = &round_arena } // bloons cannot survive past the end of round
        };
    }

    // `child_num` is 0-indexed
    Bloon get_child(u32 child_num, bool add_own_hp = false) const {
        assert(child_num < proto->children.size);
        Proto* child_proto = proto->children[child_num];
        TinyVec<Effect, 4> child_effects = { .arena = &round_arena };
        for (u32 i = 0; i < effects.size; i++) {
            if (effects[i].can_pass_to_child(get_type(), child_proto->type)) { // Note, uses the base child type without any effects applied; should not matter at all, but would be recursive problem otherwise
                child_effects.push(effects[i].pass_to_child());
            }
        }
        Bloon child = Bloon {
            .pos = pos,
            .bft = bft.get_child(proto->children.size, child_num),
            .type = child_proto->type | (child_proto->inherit_types & type), // Note, this uses the base type, not including any that were added with effects, since effects are responsible for soaking to children
            .dir = dir,
            .hp = add_own_hp ? child_proto->hp + hp : child_proto->hp,
            .movement = movement,
            .proto = child_proto,
            .effects = child_effects
        };
        child.movement.speed = child_proto->speed;
        return child;
    }

    u32 num_children() const { return proto->children.size; }
    void move() { movement.move(pos, dir); }
    bool is_exited() const { return movement.is_exited(); }
    bool is_dead() const { return hp <= 0; }
    // assuming `this` has been spawned at the place of its parent, and it's a `child_num`, regress the bloon along the track accordingly
    // TODO regress instead of progress
    void move_child(u32 child_num) {
        movement.move_helper(STEPS_CHILD_MOVE * child_num, pos, dir);
    }
    void effect_update() {
        for (i32 i = 0; i < effects.size; i++) {
            if (effects[i].is_dot_ready()) {
                hp = max(0, hp - effects[i].proto->dot_damage); // TODO for now, DOT may not overkill any bloons
                effects[i].refresh_dot();
            }
            if (effects[i].is_expired()) {
                effects.remove_swap(i);
                i--;
            }
        }
        calculate_cache_cumulative_effect();
    }
    void calculate_cache_cumulative_effect() {
        EffectProto cum = EffectProto::unit();
        for (u32 i = 0; i < effects.size; i++) {
            cum = cum + *effects[i].proto;
        }
        cached_cumulative_effect = cum;
    }
    Type get_type() const {
        return (type - cached_cumulative_effect.remove_type) | cached_cumulative_effect.add_type;
    }
};

};