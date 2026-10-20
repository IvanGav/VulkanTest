#pragma once

#include "tower.h"

Vec<nbloon::Proto> bloonProtos;
Vec<nproj::Proto> projProtos;
Vec<nattack::Proto*> attackProtos;
Vec<ntower::Proto> towerProtos;
npath::Path path;
Vec<ntower::Tower> towers;

void updateTick() {
    for (nbloon::Bloon& bloon : bloons->bloons) bloon.move();
    for (nproj::Projectile& proj : projs->list) proj.move();
    for (ntower::Tower& tower : towers) tower.tick();
    Vec<nbloon::BID> poppedBloons{ .arena = &frame_arena }; poppedBloons.reserve(255);
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
                    u32 oldHp = bloon.hp;
                    bloon.hp -= proj.proto->damage;
                    proj.pierce -= 1;
                    projs->record_hit(pi, bloon.bft);
                    if (bloon.hp <= 0 && oldHp > 0) { poppedBloons.push(bloon.bid); }
                } else {
                    proj.pierce = 0;
                }
            }
        }
    }
    for (nbloon::BID bid : poppedBloons) bloons->pop(bid);
    for (u32 i = 0; i < projs->list.size; i++) {
        if (projs->list[i].pierce == 0 || projs->list[i].lifetime_ticks == 0) {
            projs->del(i); i--;
        }
    }
}