#pragma once

#include "tlist.h"
#include "path.h"
#include "global.h"

// TODO move into `prelude.h` and get out of here
npath::Path path;

struct {
    engine::TextureRef monke;
    engine::TextureRef kyaru;
    engine::TextureRef triangle;
    engine::TextureRef projectile;
    engine::TextureRef ground;
} textures;

struct {
    engine::MeshRef monke;
    engine::MeshRef quad;
} meshes;

void updateTick() {
    for (nbloon::Bloon& bloon : bloons->list) bloon.move();
    for (nproj::Projectile& proj : projs->list) proj.move();
    for (ntower::Tower& tower : towers->list) tower.tick();
    // TODO make parallel - this is *the* hot loop of the entire game
    // TODO do some spatial acceleration too, because, again, this is the only O(n*m) loop for n=#bloons, m=#projs

    /*
    * TODO rework the hit detection system to be compatible with multithreading and gameplay consistency
    * 1. Do not modify both bloons and projectiles in the loop
    * 2. (For each thread) Record a list of bloon-projectile hits in main loop
    *   - Potentially `O(n*m)`, but rarely to be *too* bad
    *   - If runtime is such a concern, make a list of currently recorded hit amount per projectile (per frame) and stop *recording* when pierce limit is reached
    *   - Spatial acceleration should be used here, and constructed before step 1
    * 3. In the loop, whenever detecting projectile-cannot-pop-bloon hit, raise a flag to invalidate all of previous recorded hits and set pierce to 0 (aka kill the projectile)
    * 4. (Sequentially) Go over all (non-invalidated) hits and apply them (deterministically).
    *   - Since this process modifies both bloons and projectiles, it cannot be done in parallel
    *   - Technically, can split this into two steps: subtract pierce in parallel, then subtract hp (in parallel? im not sure if it's really all that possible since im sorting by either projectile OR bloon and not both, so cannot guarantee non-overlapping work without additional O(n*log(n)) sort...)
    * 5. Final (potentially negative = overkilled) hp is used to produce children
    * 6. Any time an event is triggered on bloon pop, it must signal whether it should cause a state re-evaluation
    *   - For example, pop and awe will damage all bloons on bloon about-to-exit, so must recalculate popping bloons with new damage numbers
    *   - For example, counting a pop towards a necromancer monkey does not change any actual bloon state, so no need to reclaculate
    *   - Apparently bloons spawn backwards from the parent's position, so you cannot lose by popping a bloon into many children
    *   - So long as the called events do not modify same data, they can be called in parallel
    */

    for (nbloon::Bloon& bloon : *bloons) {
        for (u32 pi = 0; pi < projs->list.size; pi++) {
            nproj::Projectile& proj = projs->list[pi];
            if (
                proj.pierce > 0 &&
                bloon.proto->hitbox.intersect(&proj.proto->hitbox, bloon.pos, proj.pos, bloon.dir, proj.dir) &&
                !projs->has_hit(pi, bloon.bft) &&
                proj.can_hit(bloon.type)
            ) {
                if (proj.can_pop(bloon.type)) {
                    bloon.hp -= proj.proto->damage; // TODO do additional damage calculations
                    proj.pierce -= 1;
                    if (!proj.has_flag(nproj::FlagsBit::SkipRecordHits)) projs->record_hit(pi, bloon.bft);
                } else {
                    proj.pierce = 0;
                }
            }
        }
    }
    for (u32 i = 0; i < projs->list.size; i++) {
        if (projs->list[i].pierce == 0 || projs->list[i].lifetime_ticks == 0) {
            projs->del(i); i--;
        }
    }
    // Pop bloons in a loop, until no event handler has (potentially) changed the game state
    // Note: there are no event handlers currently present, this is just structuring the code for the future, since I'm pretty sure it's unavoidable for my base requirements
    bool state_changed = true;
    while (state_changed) {
        state_changed = false;
        for (nbloon::Bloon const& bloon : bloons->list) {
            if (bloon.is_exited()) {
                // TODO additional logic on bloon exited
                bloons->del(bloon.id);
            }
            else if (bloon.is_dead()) {
                // TODO additional logic for overkill
                for (u32 i = 0; i < bloon.num_children(); i++) {
                    bloons->add(bloon.get_child(i));
                }
                bloons->del(bloon.id);
            }
        }
    }
}

// Remove tower with tid `id` and handles game logic for removing the tower
void remove_tower(TID id) {
    ntower::Tower* tower = towers->find(id);
    if (tower == nullptr) { warn("tried removing a non existing tower"); return; }
    // remove all children, remove self from the parent if any
    if (tower->parent != TID{}) {
        ntower::Tower* parent = towers->find(tower->parent);
        if (parent == nullptr) { warn("tower had an non existing parent, aborting removal"); return; }
        if (parent->children.remove_first_of(id)) { warn("parent did not have this tower as child, aborting removal"); return; }
    }
    for (TID const& child_id : tower->children) {
        remove_tower(child_id);
    }
    towers->del(id);
}