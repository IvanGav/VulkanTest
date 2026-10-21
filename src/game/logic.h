#pragma once

#include "tower.h"

Vec<nbloon::Proto> bloonProtos;
Vec<nproj::Proto> projProtos;
Vec<nattack::Proto*> attackProtos;
Vec<ntower::Proto> towerProtos;
npath::Path path;
Vec<ntower::Tower> towers;

struct {
    engine::TextureRef monke;
    engine::TextureRef kyaru;
    engine::TextureRef triangle;
    engine::TextureRef projectile;
    engine::TextureRef ground;
} textures;

struct {
    engine::MeshRef monke;
    engine::MeshRef testQuad;
} meshes;

void initTestPrototypes() {
    /* load textures and meshes */
    {
        textures.monke = engine::loadTexture("asset/monke.png");
        textures.kyaru = engine::loadTexture("asset/kyaru.png");
        textures.triangle = engine::loadTexture("asset/triangle.png");
        textures.projectile = engine::loadTexture("asset/projectile.png");
        textures.ground = engine::loadTexture("asset/ground.png");
    }
    {
        meshes.monke = engine::loadMeshObj("asset/monke.obj");
        meshes.testQuad = engine::loadTestMesh();
    }
    /* create projectile protos */
    projProtos.push(nproj::Proto{
        .damage = 1,
        .pierce = 3,
        .move = nmove::Move{
            .type = nmove::Type::MoveSimple,
            .move_simple = nmove::MoveSimple{.speed = 0.2 }
        },
        .hitbox = hitbox::HB{
            .type = hitbox::Type::Circle,
            .circle = hitbox::Circle{.r = 0.5 }
        },
        .lifetime_ticks = 100
        });
    projProtos.push(nproj::Proto{
        .damage = 4,
        .pierce = 1,
        .move = nmove::Move{
            .type = nmove::Type::MoveSimple,
            .move_simple = nmove::MoveSimple{.speed = 0.04 }
        },
        .hitbox = hitbox::HB{
            .type = hitbox::Type::Circle,
            .circle = hitbox::Circle{.r = 0.5 }
        },
        .lifetime_ticks = 500
        });
    /* create bloon protos */
    bloonProtos.push(nbloon::Proto{
        .hp = 1,
        .type = 0,
        .speed = 0.1,
        .children = {.data = nullptr, .size = 0 },
        .hitbox = hitbox::HB{.type = hitbox::Type::Circle, .circle = hitbox::Circle{.r = 0.5}},
        .max_hitbox_dist = 0.5,
        .speed_status_immune = false
        });
    bloonProtos.push(nbloon::Proto{
        .hp = 2,
        .type = 1,
        .speed = 0.2,
        .children = {.data = &bloonProtos.data, .size = 1 },
        .hitbox = hitbox::HB{.type = hitbox::Type::Circle, .circle = hitbox::Circle{.r = 0.5}},
        .max_hitbox_dist = 0.5,
        .speed_status_immune = false
        });
    nbloon::Proto** b2children = global_arena.alloc<nbloon::Proto*>(3);
    b2children[0] = &bloonProtos[0];
    b2children[1] = &bloonProtos[0];
    b2children[2] = &bloonProtos[1];
    bloonProtos.push(nbloon::Proto{
        .hp = 1,
        .type = 1,
        .speed = 0.05,
        .children = {.data = b2children, .size = 3 },
        .hitbox = hitbox::HB{.type = hitbox::Type::Circle, .circle = hitbox::Circle{.r = 0.5}},
        .max_hitbox_dist = 0.5,
        .speed_status_immune = false
        });
    /* create attack protos */
    nattack::PSimple* simpleAttack1 = global_arena.alloc<nattack::PSimple>(1);
    new (simpleAttack1) nattack::PSimple(6.0f, 140, 1, 0.0f, &projProtos[0]);
    attackProtos.push(simpleAttack1);
    nattack::PSimple* simpleAttack2 = global_arena.alloc<nattack::PSimple>(1);
    new (simpleAttack2) nattack::PSimple(4.0f, 300, 6, glm::radians(120.0f), &projProtos[1]);
    attackProtos.push(simpleAttack2);
    /* create tower protos */
    towerProtos.push(ntower::Proto{
        .attacks = {.data = attackProtos.data, .size = 1 },
        .target_modes = {.data = global_arena.push(nattack::TargetMode::CloseBloon), .size = 1 },
        });
    Slice<nattack::Proto*> towerAttacks1 = { .data = global_arena.alloc<nattack::Proto*>(2), .size = 2 };
    towerAttacks1.data[0] = attackProtos[0];
    towerAttacks1.data[1] = attackProtos[1];
    towerProtos.push(ntower::Proto{
        .attacks = towerAttacks1,
        .target_modes = {.data = global_arena.push(nattack::TargetMode::CloseBloon), .size = 1 },
        });
    /* other global state initialization */
    path = npath::Path{ .nodes = Vec<Vec2>::with(&global_arena, Vec2{ -5.0, -5.0 }, Vec2{ 5.0, 5.0 }, Vec2{ -5.0, 6.0 }), .cumulative_dist = Vec<f32>::with(&global_arena, 0.0f, 14.1421356237f, 24.1920112448f) };
    bloons = global_arena.push(blist::BList::create());
    projs = global_arena.push(plist::PList::create());
    towers = Vec<ntower::Tower>{ .arena = &game_arena };
}

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