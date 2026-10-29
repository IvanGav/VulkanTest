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
    for (nbloon::Bloon const& bloon : bloons->list) {
        if (bloon.is_exited()) {
            // TODO additional logic on bloon exited
            bloons->del(bloon.id);
        }
    }
    Vec<P<BID, u32>> hits = Vec<P<BID, u32>>{ .arena = &frame_arena };
    hits.reserve(projs->list.size); // reserve some arbitrary amount to begin with
    for (nbloon::Bloon& bloon : *bloons) {
        for (u32 pi = 0; pi < projs->list.size; pi++) {
            nproj::Projectile& proj = projs->list[pi];
            if (
                proj.pierce > 0 &&
                bloon.proto->hitbox.intersect(&proj.proto->hitbox, bloon.pos, proj.pos, bloon.dir, proj.dir) &&
                proj.can_hit(bloon.type) &&
                !projs->has_hit(pi, bloon.bft)
            ) {
                if (proj.can_pop(bloon.type)) {
                    hits.push({bloon.id, pi});
                } else {
                    proj.pierce = 0; // thread safe because we discard this projectile's hits later anyway
                }
            }
        }
    }
    Vec<P<BID, u32>> overkillable_hits = Vec<P<BID, u32>>{ .arena = &frame_arena }; // TODO this makes it a bit nicer in the next step, but might be very slow
    overkillable_hits.reserve(hits.size);
    for (P<BID, u32> const& hit : hits) {
        nbloon::Bloon* bloon = bloons->find(hit.a);
        nproj::Projectile& proj = projs->list[hit.b];
        if (proj.pierce == 0) { continue; } // this projectile has hit a bloon it cannot pop and died
        proj.pierce -= 1;
        i32 actual_proj_damage = proj.proto->damage; // TODO additional calculations here
        if (proj.can_overkill(bloon->type)) {
            overkillable_hits.push(hit);
        } else {
            // safely do damage directly to bloon
            i32 damage_dealt = min(bloon->hp, actual_proj_damage);
            bloon->hp = max(0, bloon->hp - actual_proj_damage);
            ntower::Tower* t = towers->find(proj.parent_tower);
            if (t != nullptr) { t->pop_count += damage_dealt; } // attribute the true damage dealt
        }
        if (!proj.has_flag(nproj::FlagsBit::SkipRecordHits)) { projs->record_hit(hit.b, bloon->bft); }
    }
    // This loop is horrendous; it's required to attribute overkilling pops to correct towers; if attributing the pops was not necessary, it could be two very simple and beautiful for loops
    u32 ok_i = 0; // OverKillable Index
    for (nbloon::Bloon& bloon : bloons->list) {
        Vec<nbloon::Bloon> children = Vec<nbloon::Bloon>{ .arena = &frame_arena };
        // this check is needed for blimps that were killed by non-overkillable projectiles
        if (bloon.is_dead()) {
            // cannot be overkilled at this point
            for (u32 i = 0; i < bloon.num_children(); i++) {
                children.push(bloon.get_child(i));
            }
        } else {
            children.push(bloon);
        }
        // the array is guaranteed to be (implicitly) grouped with all same BIDs together
        for (; ok_i < overkillable_hits.size && overkillable_hits[ok_i].a == bloon.id; ok_i += 1) {
            u32 proj_i = overkillable_hits[ok_i].b;
            nproj::Projectile& proj = projs->list[proj_i];
            i32 actual_proj_damage = proj.proto->damage; // TODO additional calculations here
            i32 damage_dealt = 0;
            for (nbloon::Bloon& b : children) {
                damage_dealt += min(b.hp, actual_proj_damage); // attribute the damage dealt to only this layer
                b.hp -= actual_proj_damage;
            }
            for (u32 i = 0; i < children.size;) {
                if (children[i].is_dead()) {
                    for (u32 j = 0; j < children[i].num_children(); j++) {
                        children.push(children[i].get_child(j, true));
                        nbloon::Bloon const& child_ref = children.back();
                        damage_dealt += child_ref.proto->hp - max(child_ref.hp, 0);
                    }
                    children.remove(i);
                } else {
                    i += 1;
                }
            }
            ntower::Tower* t = towers->find(proj.parent_tower);
            if (t != nullptr) { t->pop_count += damage_dealt; } // attribute the true damage dealt
        }
        if (children.size == 1 && children[0].id == bloon.id) {
            // has not been killed; replace with new state
            bloon = children[0];
        } else {
            // the bloon has been killed; remove it and add its children
            bloons->del(bloon.id);
            // TODO the order of bloons is deterministic but kind of messed up and unintuitive; fix later
            for (u32 i = 0; i < children.size; i++) {
                children[i].move_child(i);
                bloons->add(children[i]);
            }
        }
    }
    for (u32 i = 0; i < projs->list.size; i++) {
        if (projs->list[i].pierce == 0 || projs->list[i].lifetime_ticks == 0) {
            projs->del(i); i--;
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