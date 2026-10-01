#pragma once

#include "prelude.h"
#include "hitbox.h"
#include "move.h"

namespace bloon {

// bitmask for lead/purple/white/black/etc
struct Type {
    typedef u16 TypeU;
    TypeU val;

    Type operator|(Type other) { return Type { .val = (TypeU)(this->val | other.val) }; }
    Type operator&(Type other) { return Type { .val = (TypeU)(this->val & other.val) }; }
    Type operator-(Type other) { return Type { .val = (TypeU)(this->val & ~other.val) }; } // bitclear
    bool has_each(Type other) { return (val & other.val) == other.val; }
    bool has_any(Type other) { return (val & other.val) != (TypeU)(0); }
};

struct Proto {
    u32 hp;
    Type type;
    f32 speed;
    Slice<Proto*> children;
    hitbox::HB hitbox;
    bool speed_status_immune;
};

// Bloon Family Tree
// Specifies parent-child relationship between bloons
struct BFT {
    u32 tree;
    u32 family;
    u8 layer;
    static u32 family_count;

    static BFT spawn() {
        BFT::family_count++;
        return BFT { .tree = 0, .family = BFT::family_count, .layer = 0 };
    }
    static void reset_family_count() { BFT::family_count = 0; }
    static BFT null() { return BFT {.family = 0}; }
    bool is_null() { return family == 0; }
    
    // Return true iff `self` is a parent or a child of `other`. That means, if self and `other` are in the same subtree.
    bool same_subtree_as(BFT other) const {
        if(family != other.family) { return false; }
        u8 min_layer = min(this->layer, other.layer);
        u64 mask = first_n_bits_mask(min_layer);
        return (tree & mask) == (other.tree & mask);
    }

    // Given that this FT will produce `child_count` children total, get `child_num` child's FT
    // `child_num` is 0-indexed
    BFT get_child(u8 child_count, u8 child_num) {
        assert(child_count > 0);
        BFT child = *this; // clone self
        if(child_count == 1) {
            assert(child_num == 0);
            return child;
        }
        u8 add_layer = std::bit_width(child_count) - 1; // (TODO) suboptimal; BADs will use up extra space; doesn't matter for now
        child.tree |= ((u32) child_num) << child.layer;
        child.layer += add_layer;
        return child;
    }

    bool operator==(const BFT& other) const {
        return family == other.family && tree == other.tree && layer == other.layer;
    }
};

// Bloon ID
// Bevy's "Entity" equivalent; Used to index into "Slot Map" `BList` (declared in `blist.h`)
struct BID {
    u32 i;
    u32 gen;
};

// represents a Bloon entity
struct Bloon {
    Vec2 pos;
    hitbox::Circle min_hb; // minimal hitbox; may be used as initial "likely" hit (and true hit when `proto->hitbox` is `hitbox::Circle`)
    BID bid;
    BFT bft;
    Type type; // type can sometimes change dynamically
    f32 dir;
    i32 hp; // current hp
    move::MoveAlongRoad move;
    Proto* proto;

    static Bloon spawn(Proto* proto, Path* path) {
        return Bloon {
            .pos = path->nodes[0],
            .bid = {},
            .bft = BFT::spawn(),
            .type = proto->type,
            .dir = (path->nodes[1] - path->nodes[0]).dir(),
            .hp = (i32) proto->hp,
            .move = move::MoveAlongRoad { .waypoint = path->nodes[0], .target_node = 1, .road_pos = 0.0, .speed = proto->speed, .path = path },
            .proto = proto,
        };
    }

    // `child_num` is 0-indexed
    Bloon get_child(u32 child_num) {
        assert(child_num < proto->children.size);
        Proto* child_proto = proto->children[child_num];
        Bloon child = Bloon {
            .pos = pos,
            .bft = bft.get_child(proto->children.size, child_num),
            .type = child_proto->type,
            .dir = dir,
            .hp = (i32) child_proto->hp,
            .move = move, // temporary
            .proto = child_proto
        };
        child.move.move_helper(child.move.speed * child_num, child.pos, child.dir); // move the child based on child_num
        child.move.speed = child_proto->speed;
        return child;
    }

    bool is_dead() { return hp <= 0; }
    void move() { move.move(pos, dir); }
    bool is_exited() { return move.is_exited(); }
};

};