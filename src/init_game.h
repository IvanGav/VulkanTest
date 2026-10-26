#pragma once

#include "game/logic.h"

void load_textures() {
    textures.monke = engine::loadTexture("asset/monke.png");
    textures.kyaru = engine::loadTexture("asset/kyaru.png");
    textures.triangle = engine::loadTexture("asset/triangle.png");
    textures.projectile = engine::loadTexture("asset/projectile.png");
    textures.ground = engine::loadTexture("asset/ground.png");
}

void load_meshes() {
    meshes.monke = engine::loadMeshObj("asset/monke.obj");
    meshes.quad = engine::loadTestMesh();
}

void init_projectile_protos() {
    proj_protos.push(nproj::Proto{
        .damage = 1,
        .pierce = 3,
        .move = nmove::Move{ .type = nmove::Type::MoveSimple, .move_simple = nmove::MoveSimple{ .speed = 0.2 } },
        .hitbox = hitbox::HB::make_circle(0.5),
        .lifetime_ticks = 100
        });
    proj_protos.push(nproj::Proto{
        .damage = 4,
        .pierce = 1,
        .move = nmove::Move{ .type = nmove::Type::MoveSimple, .move_simple = nmove::MoveSimple{ .speed = 0.04 } },
        .hitbox = hitbox::HB::make_circle(0.5),
        .lifetime_ticks = 500
        });
}

void init_bloon_protos() {
    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = 0,
        .speed = 0.1,
        .children = { .data = nullptr, .size = 0 },
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .speed_status_immune = false
        });
    Slice<nbloon::Proto*> b1children = Vec<nbloon::Proto*>::with(&global_arena, &bloon_protos[0]).full_slice();
    bloon_protos.push(nbloon::Proto{
        .hp = 2,
        .type = 1,
        .speed = 0.2,
        .children = b1children,
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .speed_status_immune = false
        });
    Slice<nbloon::Proto*> b2children = Vec<nbloon::Proto*>::with(&global_arena, &bloon_protos[0], &bloon_protos[0], &bloon_protos[1]).full_slice();
    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = 1,
        .speed = 0.05,
        .children = b2children,
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .speed_status_immune = false
        });
}

void init_attack_protos() {
    attack_protos.push(global_arena.alloc<nattack::PSimple>(1));
    new (attack_protos.back()) nattack::PSimple(6.0f, 140, 1, 0.0f, &proj_protos[0]);
    attack_protos.push(global_arena.alloc<nattack::PSimple>(1));
    new (attack_protos.back()) nattack::PSimple(4.0f, 300, 6, glm::radians(120.0f), &proj_protos[1]);
}

void init_tower_protos() {
    tower_protos.push(ntower::Proto{
        .attacks = { .data = &attack_protos[0], .size = 1},
        .target_modes = { .data = global_arena.push(nattack::TargetMode::CloseBloon), .size = 1 },
        });
    Slice<nattack::Proto*> t1attacks = Vec<nattack::Proto*>::with(&global_arena, attack_protos[0], attack_protos[1]).full_slice();
    tower_protos.push(ntower::Proto{
        .attacks = t1attacks,
        .target_modes = { .data = global_arena.push(nattack::TargetMode::CloseBloon), .size = 1 },
        });
}

/* Call this function to load textures/meshes, initalize the game prototypes, reset the bloons/projs/towers/path */

void initTestPrototypes() {
    load_textures();
    load_meshes();
    init_projectile_protos();
    init_bloon_protos();
    init_attack_protos();
    init_tower_protos();
    /* other global state initialization */
    path = npath::Path{ .nodes = Vec<Vec2>::with(&global_arena, Vec2{ -5.0, -5.0 }, Vec2{ 5.0, 5.0 }, Vec2{ -5.0, 6.0 }), .cumulative_dist = Vec<f32>::with(&global_arena, 0.0f, 14.1421356237f, 24.1920112448f) };
    bloons = global_arena.push(blist::BList::create());
    projs = global_arena.push(plist::PList::create());
    towers = global_arena.push(tlist::TList::create());
}