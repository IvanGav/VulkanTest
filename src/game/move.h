#pragma once

#include "prelude.h"
#include "path.h"

namespace move {

// /*
//     Movement modifiers
// */

// /// A movement modifier component that lets an entity to steer a constant amount every tick
// /// That means turning while preserving the total velocity
// struct ConstSteerMoveMod {
//     f32 steer_angle; // between -0.5 and 0.5, where 0.0 is no steer and 0.25 is steer 90 degrees right
// };
// /// A movement modifier component that lets an entity to steer towards the target
// /// That means turning while preserving the total velocity
// struct SteerMoveMod {
//     f32 steer_str;
//     Vec2 waypoint; // where to steer towards
//     BID target; // used to update the `waypoint` every tick
// };
// /// A movement modifier component that lets an entity to home towards the target
// /// That means accelerating to the direction of target, up to a max velocity
// struct HomeMoveMod {
//     f32 home_str;
//     f32 max_velocity;
//     Vec2 waypoint; // where to home towards
//     BID target; // used to update the `waypoint` every tick
// };
// /// A movement modifier component that lets an entity to sharply change own velocity after `hit_flag` has been set
// /// While default behavior is to go towards the closest bloon, it can be modified if needed
// struct SeekAfterHitMoveMod {
//     bool hit_flag; // set after projectile has hit a bloon; TODO may not be necessary at all
// };

// /*
//     Union movement modifier struct (only apply to simple move)
// */

// enum class MoveModType {
//     None, ConstSteerMove, SteerMove, HomeMove, SeekAfterHitMove
// };

// struct MoveMod {
//     MoveModType type;
//     union {
//         ConstSteerMoveMod const_steer;
//         SteerMoveMod steer;
//         HomeMoveMod home;
//         SeekAfterHitMoveMod seek_after_hit;
//     };
// };

/*
    Movement types
*/

// A movement component that lets an entity to move along the road
struct MoveAlongRoad {
    Vec2 waypoint; // move to this position (not required to be on the road)
    u32 target_node; // when reaching the waypoint, increment by 1
    f32 road_pos; // position along the road, mostly for "first" and "last" targeting; may be negative
    f32 speed;
    Path* path;

    void move(Vec2& pos_mut, f32& dir_mut) {
        this->move_helper(speed, pos_mut, dir_mut);
    }

    // TODO change directions on turn
    // Technically recursive, but will not recurse more than once unless big speed or small path node dist
    void move_helper(f32 step, Vec2& pos_mut, f32& dir_mut) {
        f32 dx = waypoint.x - pos_mut.x;
        f32 dy = waypoint.y - pos_mut.y;
        f32 total_dist = std::hypot(dx,dy);

        if(total_dist < step) {
            // Move to the node and advance the node index
            pos_mut.x = waypoint.x;
            pos_mut.y = waypoint.y;
            target_node += 1;
            if(target_node < path->nodes.size) {
                waypoint = path->nodes[target_node];
                road_pos = path->cumulative_dist[target_node];
            } else {
                // maybe do something else; just indicate that this entity has is_exited the track
                waypoint = Vec2 { .x = F32_INF, .y = F32_INF };
                road_pos = 0.;
            }
            this->move_helper(step-total_dist, pos_mut, dir_mut);
        } else {
            pos_mut.x += dx * step / total_dist;
            pos_mut.y += dy * step / total_dist;
            road_pos += step;
        }
    }

    bool is_exited() { return waypoint == Vec2 { .x = F32_INF, .y = F32_INF }; }
};

// A movement component that lets an entity to rapidly move to a specified location and stay stationary after that
struct MoveWaypoint {
    Vec2 waypoint;
    
    void move(Vec2& pos_mut, f32& dir_mut) {
        if(pos_mut == waypoint) { return; }
        f32 dx = waypoint.x - pos_mut.x;
        f32 dy = waypoint.y - pos_mut.y;
        if(std::hypot(dx, dy) < 0.1) { pos_mut = waypoint; return; }
        pos_mut.x = dx / 8.0;
        pos_mut.y += dy / 8.0;
    }
};

// A movement component that lets an entity to move in a straight line (add move modifier components to change direction)
struct MoveSimple {
    Vec2 velocity;
    u32 bounce; // number of bounces left
    bool collide; // if true, collide with obstacles higher than self

    // TODO change directions on turn or bounce
    void move(Vec2& pos_mut, f32& dir_mut) {
        pos_mut.x += velocity.x;
        pos_mut.y += velocity.y;
    }
};

/*
    Union movement struct
*/

enum class Type {
    MoveAlongRoad, MoveSimple, MoveWaypoint
};

struct Move {
    Type type;
    union {
        MoveAlongRoad move_road;
        MoveWaypoint move_waypoint;
        MoveSimple move_simple;
    };

    void move(Vec2& pos_mut, f32& dir_mut) {
        switch(type) {
            case Type::MoveAlongRoad: move_road.move(pos_mut, dir_mut); break;
            case Type::MoveSimple: move_simple.move(pos_mut, dir_mut); break;
            case Type::MoveWaypoint: move_waypoint.move(pos_mut, dir_mut); break;
        }
    }
};

}