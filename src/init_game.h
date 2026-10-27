#pragma once

#include "game/logic.h"

/* attack functions */

template <f32 range, u32 count, f32 spread>
bool multishot_attack(ntower::Tower* tower, nproj::Buff const& buff_context, nproj::Proto* proj) {
    assert(tower->get_target_mode() == TargetMode::CloseBloon); // TODO
    nbloon::Bloon const* target = nullptr;
    f32 closest_dist = F32_INF;
    for (nbloon::Bloon const& bloon : bloons->list) {
        f32 dist = (bloon.pos - tower->pos).len();
        if (dist < closest_dist && dist <= range) {
            closest_dist = dist;
            target = &bloon;
        }
    }
    // fire
    if (target == nullptr) { return false; }
    tower->dir = (target->pos - tower->pos).dir();
    if (count == 1) {
        projs->add(nproj::Projectile::spawn(proj, buff_context, tower->pos, tower->dir));
    }
    else {
        for (u32 i = 0; i < count; i++) {
            f32 gap_angle = spread / (count - 1);
            f32 proj_dir = tower->dir - spread / 2 + i * gap_angle;
            projs->add(nproj::Projectile::spawn(proj, buff_context, tower->pos, proj_dir));
        }
    }
    return true;
};

template <f32 range>
bool simple_attack(ntower::Tower* tower, nproj::Buff const& buff_context, nproj::Proto* proj) {
    return multishot_attack<range, 1, 0.0f>(tower, buff_context, proj);
}

/* loading/initializing functions */

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

void init_tower_protos() {
    Slice<ntower::Attack> t0attacks = Vec<ntower::Attack>::with(&global_arena,
        ntower::Attack{
            .attack = std::function([proj_proto = &proj_protos[0]](ntower::Tower* tower, nproj::Buff const& buff_context) {
                return simple_attack<6.0f>(tower, buff_context, proj_proto);
            }),
            .cooldown_ticks = 140
        }
    ).full_slice();
    tower_protos.push(ntower::Proto{
        .attacks = t0attacks,
        .target_modes = { .data = global_arena.push(ntower::TargetMode::CloseBloon), .size = 1 },
        });

    Slice<ntower::Attack> t1attacks = Vec<ntower::Attack>::with(&global_arena, 
        ntower::Attack{
            .attack = std::function([proj_proto = &proj_protos[0]](ntower::Tower* tower, nproj::Buff const& buff_context) {
                return simple_attack<6.0f>(tower, buff_context, proj_proto);
            }),
            .cooldown_ticks = 140
        },
        ntower::Attack{
            .attack = std::function([proj_proto = &proj_protos[1]](ntower::Tower* tower, nproj::Buff const& buff_context) {
                return multishot_attack<4.0f, 6, glm::radians(120.0f)>(tower, buff_context, proj_proto);
            }),
            .cooldown_ticks = 300
        }
    ).full_slice();
    tower_protos.push(ntower::Proto{
        .attacks = t1attacks,
        .target_modes = { .data = global_arena.push(ntower::TargetMode::CloseBloon), .size = 1 },
        });
}

/* Call this function to load textures/meshes, initalize the game prototypes, reset the bloons/projs/towers/path */

void initTestPrototypes() {
    load_textures();
    load_meshes();
    init_bloon_protos();
    init_projectile_protos();
    init_tower_protos();
    /* other global state initialization */
    path = npath::Path{ .nodes = Vec<Vec2>::with(&global_arena, Vec2{ -5.0, -5.0 }, Vec2{ 5.0, 5.0 }, Vec2{ -5.0, 6.0 }), .cumulative_dist = Vec<f32>::with(&global_arena, 0.0f, 14.1421356237f, 24.1920112448f) };
    bloons = global_arena.push(blist::BList::create());
    projs = global_arena.push(plist::PList::create());
    towers = global_arena.push(tlist::TList::create());
}

/* dart monke */

void initDartMonkey() {
    // 63 crosspath options per monkey... that's a lot

    // projectiles
    // dart 000
    proj_protos.push(nproj::Proto{
        .damage = 1,
        .pierce = 2,
        .cannot_hit_type = BTYPE_CAMO,
        .cannot_pop_type = PTYPE_SHARP,
        .move = nmove::Move::make_simple(0.4f), // TODO arbitrary
        .hitbox = hitbox::HB::make_circle(1.0f), // TODO arbitrary
        .lifetime_ticks = data::secondsToUpdates(2.0), // TODO arbitrary
    });
    nproj::Proto* dart000proj = &proj_protos.back();
    // dart 100
    proj_protos.push(*dart000proj);
    nproj::Proto* dart100proj = &proj_protos.back();
    dart100proj->pierce += 1;
    // dart 200
    proj_protos.push(*dart000proj);
    nproj::Proto* dart200proj = &proj_protos.back();
    dart200proj->pierce += 3;
    // dart 010
    nproj::Proto* dart010proj = dart000proj;
    // dart 020
    nproj::Proto* dart020proj = dart000proj;
    // dart 001
    proj_protos.push(*dart000proj);
    nproj::Proto* dart001proj = &proj_protos.back();
    dart001proj->lifetime_ticks *= 1.35;
    // dart 002
    proj_protos.push(*dart000proj);
    nproj::Proto* dart002proj = &proj_protos.back();
    dart002proj->lifetime_ticks *= 1.6;
    dart002proj->move.move_simple.speed *= 1.1667;
    dart002proj->cannot_hit_type = {}; // can hit all types
    // dart 110
    // dart 101
    // dart 011
    // dart 210
    // dart 201
    // dart 120
    // dart 021
    // dart 102
    // dart 012
    // dart 220
    // dart 202
    // dart 022
    // dart 300
    // dart 310
    // dart 320
    // dart 301
    // dart 302
    // dart 400
    // dart 410
    // dart 420
    // dart 401
    // dart 402
    // dart 500
    // dart 510
    // dart 520
    // dart 501
    // dart 502
    // dart 030
    // dart 130
    // dart 230
    // dart 031
    // dart 032
    // dart 040
    // dart 140
    // dart 240
    // dart 041
    // dart 042
    // dart 050
    // dart 150
    // dart 250
    // dart 051
    // dart 052
    // dart 003
    // dart 103
    // dart 203
    // dart 013
    // dart 023
    // dart 004
    // dart 104
    // dart 204
    // dart 014
    // dart 024
    // dart 005
    // dart 105
    // dart 205
    // dart 015
    // dart 025

    // towers
    Slice<ntower::TargetMode> base_target_modes = Vec<ntower::TargetMode>::with(&global_arena, ntower::TargetMode::FirstBloon, ntower::TargetMode::LastBloon, ntower::TargetMode::CloseBloon, ntower::TargetMode::StrongBloon).full_slice();
    // dart 000
    Slice<ntower::Attack> dart000attacks = Vec<ntower::Attack>::with(&global_arena, ntower::Attack{ .attack = std::function([proj_proto = dart000proj](ntower::Tower* tower, nproj::Buff const& buff_context) { return simple_attack<range_btd_to_world(32.0f)>(tower, buff_context, proj_proto); }), .cooldown_ticks = data::secondsToUpdates(0.95)}).full_slice();
    tower_protos.push(ntower::Proto{ .category = ntower::Category::Primary, .type = TTYPE_DART_MONKEY, .attacks = dart000attacks, .target_modes = base_target_modes });
    // dart 100
    // dart 200
    // dart 010
    // dart 020
    // dart 001
    // dart 002
    // dart 110
    // dart 101
    // dart 011
    // dart 210
    // dart 201
    // dart 120
    // dart 021
    // dart 102
    // dart 012
    // dart 220
    // dart 202
    // dart 022
    // dart 300
    // dart 310
    // dart 320
    // dart 301
    // dart 302
    // dart 400
    // dart 410
    // dart 420
    // dart 401
    // dart 402
    // dart 500
    // dart 510
    // dart 520
    // dart 501
    // dart 502
    // dart 030
    // dart 130
    // dart 230
    // dart 031
    // dart 032
    // dart 040
    // dart 140
    // dart 240
    // dart 041
    // dart 042
    // dart 050
    // dart 150
    // dart 250
    // dart 051
    // dart 052
    // dart 003
    // dart 103
    // dart 203
    // dart 013
    // dart 023
    // dart 004
    // dart 104
    // dart 204
    // dart 014
    // dart 024
    // dart 005
    // dart 105
    // dart 205
    // dart 015
    // dart 025
}