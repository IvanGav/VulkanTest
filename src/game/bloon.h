#pragma once

#include "prelude.h"
#include "hitbox.h"
#include "move.h"

namespace nbloon {

const u32 STEPS_MULT = 20;

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
    f32 max_hitbox_dist; // distance to farthest point on a hitbox; basically defines the AABB of this hitbox
    bool speed_status_immune;
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
    bool is_null() { return family == 0; }
    
    // Return true iff `self` is a parent or a child of `other`. That means, if self and `other` are in the same subtree.
    bool same_subtree_as(BFT other) const {
        if(family != other.family) { return false; }
        u8 min_layer = min(layer, other.layer);
        u32 mask = u32(first_n_bits_mask(min_layer));
        std::cout << "    min layer=" << u32(min_layer) << ", mask=" << mask << ", tree1 & mask=" << (tree & mask) << ", tree2 & mask=" << (other.tree & mask) << std::endl;
        return (tree & mask) == (other.tree & mask);
    }

    // Given that this FT will produce `child_count` children total, get `child_num` child's FT
    // `child_num` is 0-indexed
    BFT get_child(u8 child_count, u8 child_num) {
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
    nmove::MoveAlongRoad movement;
    Proto* proto;

    static Bloon spawn(Proto* proto, npath::Path* path) {
        return Bloon {
            .pos = path->nodes[0],
            .bft = BFT::spawn(),
            .type = proto->type,
            .dir = (path->nodes[1] - path->nodes[0]).dir(),
            .hp = (i32) proto->hp,
            .movement = nmove::MoveAlongRoad { .waypoint = path->nodes[1], .target_node = 1, .road_pos = 0.0, .speed = proto->speed, .path = path },
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
            .movement = movement, // temporary
            .proto = child_proto
        };
        child.movement.move_helper(child.movement.speed * STEPS_MULT * child_num, child.pos, child.dir); // move the child based on child_num
        child.movement.speed = child_proto->speed;
        return child;
    }

    u32 num_children() { return proto->children.size; }
    bool is_dead() { return hp <= 0; }
    void move() { movement.move(pos, dir); }
    bool is_exited() { return movement.is_exited(); }
};

};