#pragma once

#include "game/logic.h"

/* helper functions */

// put a slice of prototypes onto global arena
template <typename T, typename... Args>
auto make_global_slice(Args&&... args) {
    return Vec<T>::with(&global_arena, std::forward<Args>(args)...).full_slice();
}

/* attack functions */

template <f32 range, u32 count, f32 spread>
bool multishot_attack(ntower::Tower* tower, nproj::Buff const& buff_context, nproj::Proto* proj) {
    assert(tower->get_target_mode() == ntower::TargetMode::CloseBloon); // TODO
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
    if constexpr (count == 1) {
        projs->add(nproj::Projectile::spawn(proj, buff_context, tower->pos, tower->dir, tower->id));
    }
    else {
        for (u32 i = 0; i < count; i++) {
            f32 gap_angle = spread / (count - 1);
            f32 proj_dir = tower->dir - spread / 2 + i * gap_angle;
            projs->add(nproj::Projectile::spawn(proj, buff_context, tower->pos, proj_dir, tower->id));
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
    textures.monke2 = engine::loadTexture("asset/monke2.png");
    textures.kyaru = engine::loadTexture("asset/kyaru.png");
    textures.triangle = engine::loadTexture("asset/triangle.png");
    textures.projectile = engine::loadTexture("asset/projectile.png");
    textures.ground = engine::loadTexture("asset/ground.png");
    textures.moab = engine::loadTexture("asset/moab.png");
    textures.bfb = engine::loadTexture("asset/bfb.png");
    textures.zomg = engine::loadTexture("asset/zomg.png");
    textures.ddt = engine::loadTexture("asset/ddt.png");
    textures.bad = engine::loadTexture("asset/bad.png");

    textures.red = engine::loadTexture("asset/bloon/red.png");
    textures.blue = engine::loadTexture("asset/bloon/blue.png");
    textures.green = engine::loadTexture("asset/bloon/green.png");
    textures.yellow = engine::loadTexture("asset/bloon/yellow.png");
    textures.pink = engine::loadTexture("asset/bloon/pink.png");
    textures.black = engine::loadTexture("asset/bloon/black.png");
    textures.white = engine::loadTexture("asset/bloon/white.png");
    textures.zebra = engine::loadTexture("asset/bloon/zebra.png");
    textures.lead = engine::loadTexture("asset/bloon/lead.png");
    textures.rainbow = engine::loadTexture("asset/bloon/rainbow.png");
    textures.urple = engine::loadTexture("asset/bloon/urple.png");
    textures.ceramic = engine::loadTexture("asset/bloon/ceramic.png");
}

void load_meshes() {
    meshes.monke = engine::loadMeshObj("asset/monke.obj");
    meshes.moab = engine::loadMeshObj("asset/moab.obj");
    meshes.quad = engine::makeQuadMesh();
}

void init_animations() {
    std::array<u32, 3> anim1duration = {60, 60, 50};
    std::array<std::array<visual::AnimationTransitionType, 3>, 3> anim1transitions = {
        std::array<visual::AnimationTransitionType, 3>{visual::AnimationTransitionType::Disallow, visual::AnimationTransitionType::AllowWhenDone, visual::AnimationTransitionType::AllowWhenDone},
        std::array<visual::AnimationTransitionType, 3>{visual::AnimationTransitionType::Disallow, visual::AnimationTransitionType::Allow, visual::AnimationTransitionType::Allow},
        std::array<visual::AnimationTransitionType, 3>{visual::AnimationTransitionType::Disallow, visual::AnimationTransitionType::AllowWhenDone, visual::AnimationTransitionType::Allow}
    };
    std::array<visual::AnimationState, 3> anim1auto = { visual::AnimationState::Idle, visual::AnimationState::Idle, visual::AnimationState::Idle };

    visual::animation_duration_table.push(anim1duration);
    visual::animation_transition_table.push(anim1transitions);
    visual::animation_auto_transition_table.push(anim1auto);
}

void init_projectile_protos() {
    proj_protos.push(nproj::Proto{
        .damage = 1,
        .pierce = 3,
        .cannot_pop_type = PTYPE_SHARP,
        .move = nmove::Move{ .type = nmove::Type::MoveSimple, .move_simple = nmove::MoveSimple{ .speed = 0.35 } },
        .hitbox = hitbox::HB::make_circle(0.5),
        .lifetime_ticks = 100,
        .sprite = { .mesh = meshes.quad, .texture = textures.projectile }
        });
    proj_protos.push(nproj::Proto{
        .damage = 3,
        .pierce = 1,
        .move = nmove::Move{ .type = nmove::Type::MoveSimple, .move_simple = nmove::MoveSimple{ .speed = 0.15 } },
        .hitbox = hitbox::HB::make_circle(0.5),
        .lifetime_ticks = 500,
        .sprite = { .mesh = meshes.quad, .texture = textures.projectile }
        });
    proj_protos.push(nproj::Proto{
        .damage = 2,
        .pierce = 2,
        .cannot_pop_type = PTYPE_SHARP,
        .move = nmove::Move{ .type = nmove::Type::MoveSimple, .move_simple = nmove::MoveSimple{ .speed = 0.5 } },
        .hitbox = hitbox::HB::make_circle(0.5),
        .lifetime_ticks = 100,
        .sprite = {.mesh = meshes.quad, .texture = textures.projectile }
        });
    proj_protos.push(nproj::Proto{
        .damage = 1,
        .pierce = 32,
        //.cannot_pop_type = PTYPE_SHARP,
        .move = nmove::Move{ .type = nmove::Type::MoveSimple, .move_simple = nmove::MoveSimple{ .speed = 1.5 } },
        .hitbox = hitbox::HB::make_circle(0.5),
        .lifetime_ticks = 50,
        .sprite = {.mesh = meshes.quad, .texture = textures.projectile }
        });
    proj_protos.push(nproj::Proto{
        .damage = 24899,
        .pierce = 64,
        .move = nmove::Move{ .type = nmove::Type::MoveSimple, .move_simple = nmove::MoveSimple{ .speed = 3.0 } },
        .hitbox = hitbox::HB::make_circle(1.0),
        .lifetime_ticks = 50,
        .flags = nproj::Flags(nproj::FlagsBit::OverkillBlimp),
        .sprite = {.mesh = meshes.quad, .texture = textures.projectile }
        });
}

void init_bloon_protos() {
    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = 0,
        .speed = btd_to_world(25.0f),
        .children = {},
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.red, .scale = 1.0f },
        .name = "red bloon"
        });
    nbloon::Proto* red = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = 0,
        .speed = btd_to_world(35.0f),
        .children = make_global_slice<nbloon::Proto*>(red),
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.blue, .scale = 1.0f },
        .name = "blue bloon"
        });
    nbloon::Proto* blue = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = 0,
        .speed = btd_to_world(45.0f),
        .children = make_global_slice<nbloon::Proto*>(blue),
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.green, .scale = 1.0f },
        .name = "green bloon"
        });
    nbloon::Proto* green = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = 0,
        .speed = btd_to_world(80.0f),
        .children = make_global_slice<nbloon::Proto*>(green),
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.yellow, .scale = 1.0f },
        .name = "yellow bloon"
        });
    nbloon::Proto* yellow = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = 0,
        .speed = btd_to_world(87.5f),
        .children = make_global_slice<nbloon::Proto*>(yellow),
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.pink, .scale = 1.0f },
        .name = "pink bloon"
        });
    nbloon::Proto* pink = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = BTYPE_BLACK,
        .speed = btd_to_world(45.0f),
        .children = make_global_slice<nbloon::Proto*>(pink, pink),
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.black, .scale = 0.75f },
        .name = "black bloon"
        });
    nbloon::Proto* black = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = BTYPE_WHITE,
        .speed = btd_to_world(50.0f),
        .children = make_global_slice<nbloon::Proto*>(pink, pink),
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.white, .scale = 0.75f },
        .name = "white bloon"
        });
    nbloon::Proto* white = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = BTYPE_PURPLE,
        .speed = btd_to_world(75.0f),
        .children = make_global_slice<nbloon::Proto*>(pink, pink),
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.urple, .scale = 1.0f },
        .name = "purple bloon"
        });

    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = BTYPE_BLACK | BTYPE_WHITE,
        .speed = btd_to_world(45.0f),
        .children = make_global_slice<nbloon::Proto*>(black, white),
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.zebra, .scale = 1.0f },
        .name = "zebra bloon"
        });
    nbloon::Proto* zebra = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = 0,
        .speed = btd_to_world(55.0f),
        .children = make_global_slice<nbloon::Proto*>(zebra, zebra),
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.rainbow, .scale = 1.0f },
        .name = "rainbow bloon"
        });
    nbloon::Proto* rainbow = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 10,
        .type = BTYPE_CERAMIC,
        .speed = btd_to_world(62.5f),
        .children = make_global_slice<nbloon::Proto*>(rainbow, rainbow),
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.ceramic, .scale = 1.0f },
        .name = "ceramic bloon"
        });
    nbloon::Proto* ceramic = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 1,
        .type = BTYPE_LEAD,
        .speed = btd_to_world(25.0f),
        .children = make_global_slice<nbloon::Proto*>(black, black),
        .hitbox = hitbox::HB::make_circle(0.5),
        .max_hitbox_dist = 0.5,
        .sprite = {.mesh = meshes.quad, .texture = textures.lead, .scale = 1.0f },
        .name = "lead bloon"
        });

    bloon_protos.push(nbloon::Proto{
        .hp = 200,
        .type = BTYPE_BLIMP,
        .speed = btd_to_world(25.0f),
        .children = make_global_slice<nbloon::Proto*>(ceramic, ceramic, ceramic, ceramic),
        .hitbox = hitbox::HB::make_circle(1.0),
        .max_hitbox_dist = 1.0,
        .sprite = {.mesh = meshes.moab, .texture = textures.moab, .scale = 1.0f },
        .name = "moab blimp"
        });
    nbloon::Proto* moab = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 700,
        .type = BTYPE_BLIMP,
        .speed = btd_to_world(6.25f),
        .children = make_global_slice<nbloon::Proto*>(moab, moab, moab, moab),
        .hitbox = hitbox::HB::make_circle(1.5), // TODO made up: scale factor for BFB relative to MOAB
        .max_hitbox_dist = 1.5,
        .sprite = {.mesh = meshes.moab, .texture = textures.bfb, .scale = 1.5f },
        .name = "bfb blimp"
        });
    nbloon::Proto* bfb = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 4000,
        .type = BTYPE_BLIMP,
        .speed = btd_to_world(4.5f),
        .children = make_global_slice<nbloon::Proto*>(bfb, bfb, bfb, bfb),
        .hitbox = hitbox::HB::make_circle(2.0), // TODO made up: scale factor for ZOMG relative to MOAB
        .max_hitbox_dist = 2.0,
        .sprite = {.mesh = meshes.moab, .texture = textures.zomg, .scale = 2.0f },
        .name = "zomg blimp"
        });
    nbloon::Proto* zomg = &bloon_protos.back();

    bloon_protos.push(nbloon::Proto{
        .hp = 400,
        .type = BTYPE_BLIMP | BTYPE_CAMO | BTYPE_LEAD | BTYPE_BLACK,
        .speed = btd_to_world(66.0f),
        .children = make_global_slice<nbloon::Proto*>(ceramic, ceramic, ceramic, ceramic),
        .hitbox = hitbox::HB::make_circle(1.2), // TODO made up: scale factor for DDT relative to MOAB
        .max_hitbox_dist = 1.2,
        .sprite = {.mesh = meshes.moab, .texture = textures.ddt, .scale = 1.2f },
        .name = "ddt blimp"
        });
    nbloon::Proto* ddt = &bloon_protos.back();

    // rbe = 55,760
    bloon_protos.push(nbloon::Proto{
        .hp = 20000,
        .type = BTYPE_BLIMP,
        .speed = btd_to_world(4.5f),
        .children = make_global_slice<nbloon::Proto*>(zomg, zomg, ddt, ddt, ddt),
        .hitbox = hitbox::HB::make_circle(2.5), // TODO made up: scale factor for BAD relative to MOAB
        .max_hitbox_dist = 2.5,
        .sprite = {.mesh = meshes.moab, .texture = textures.bad, .scale = 2.5f },
        .name = "bad blimp"
        });
}

void init_tower_protos() {
    Slice<ntower::TargetMode> standard_target_modes = make_global_slice<ntower::TargetMode>(ntower::TargetMode::CloseBloon, ntower::TargetMode::FirstBloon, ntower::TargetMode::StrongBloon, ntower::TargetMode::LastBloon, ntower::TargetMode::CloseBloon);
    nproj::Proto* proj1d3p = &proj_protos[0];
    nproj::Proto* proj3d1p = &proj_protos[1];
    nproj::Proto* proj2d2p = &proj_protos[2];
    nproj::Proto* projop1 = &proj_protos[3];
    nproj::Proto* projop2dmg = &proj_protos[4];
    tower_protos.push(ntower::Proto{
        .attacks = make_global_slice<ntower::Attack>(
            ntower::Attack{
                .attack = new std::function([proj_proto = proj2d2p](ntower::Tower* tower, nproj::Buff const& buff_context) {
                    return simple_attack<6.0f>(tower, buff_context, proj_proto);
                }),
                .cooldown_ticks = 100
            }
        ),
        .target_modes = standard_target_modes,
        .sprite = { .mesh = meshes.monke, .texture = textures.monke, .state = visual::AnimationState::Spawn, .animation_duration_ref = 0, .animation_transition_ref = 0 },
        .tower_name = "Dart Monke"
    });
    tower_protos.push(ntower::Proto{
        .attacks = make_global_slice<ntower::Attack>(
            ntower::Attack{
                .attack = new std::function([proj_proto = proj1d3p](ntower::Tower* tower, nproj::Buff const& buff_context) {
                    return simple_attack<10.0f>(tower, buff_context, proj_proto);
                }),
                .cooldown_ticks = 25
            }
        ),
        .target_modes = standard_target_modes,
        .sprite = {.mesh = meshes.monke, .texture = textures.monke, .state = visual::AnimationState::Spawn, .animation_duration_ref = 0, .animation_transition_ref = 0 },
        .tower_name = "Minigun Monke"
    });
    tower_protos.push(ntower::Proto{
        .attacks = make_global_slice<ntower::Attack>(
        //    ntower::Attack{
        //        .attack = new std::function([proj_proto = proj1d3p](ntower::Tower* tower, nproj::Buff const& buff_context) {
        //            return simple_attack<6.0f>(tower, buff_context, proj_proto);
        //        }),
        //        .cooldown_ticks = 100
        //    },
            ntower::Attack{
                .attack = new std::function([proj_proto = proj3d1p](ntower::Tower* tower, nproj::Buff const& buff_context) {
                    return multishot_attack<4.0f, 3, glm::radians(90.0f)>(tower, buff_context, proj_proto);
                }),
                .cooldown_ticks = 200
            }
        ),
        .target_modes = standard_target_modes,
        .sprite = { .mesh = meshes.monke, .texture = textures.monke2, .state = visual::AnimationState::Spawn, .animation_duration_ref = 0, .animation_transition_ref = 0 },
        .tower_name = "Shotgun Monke"
    });
    // op towers
    tower_protos.push(ntower::Proto{
        .attacks = make_global_slice<ntower::Attack>(
            ntower::Attack{
                .attack = new std::function([proj_proto = projop1](ntower::Tower* tower, nproj::Buff const& buff_context) {
                    return multishot_attack<10.0f, 50, glm::radians(90.0f)>(tower, buff_context, proj_proto);
                }),
                .cooldown_ticks = 5
            }
        ),
        .target_modes = standard_target_modes,
        .sprite = { .mesh = meshes.monke, .texture = textures.monke2, .state = visual::AnimationState::Spawn, .animation_duration_ref = 0, .animation_transition_ref = 0 },
        .tower_name = "Op Minigun"
    });
    tower_protos.push(ntower::Proto{
        .attacks = make_global_slice<ntower::Attack>(
            ntower::Attack{
                .attack = new std::function([proj_proto = projop2dmg](ntower::Tower* tower, nproj::Buff const& buff_context) {
                    return simple_attack<10.0f>(tower, buff_context, proj_proto);
                }),
                .cooldown_ticks = 100
            }
        ),
        .target_modes = standard_target_modes,
        .sprite = { .mesh = meshes.monke, .texture = textures.monke2, .state = visual::AnimationState::Spawn, .animation_duration_ref = 0, .animation_transition_ref = 0 },
        .tower_name = "Op Railgun"
    });
}

/* Call this function to load textures/meshes, initalize the game prototypes, reset the bloons/projs/towers/path */

void initTestPrototypes() {
    load_textures();
    load_meshes();
    init_animations();
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
    Slice<ntower::Attack> dart000attacks = Vec<ntower::Attack>::with(&global_arena, ntower::Attack{ .attack = global_arena.push(std::function([proj_proto = dart000proj](ntower::Tower* tower, nproj::Buff const& buff_context) { return simple_attack<range_btd_to_world(32.0f)>(tower, buff_context, proj_proto); })), .cooldown_ticks = data::secondsToUpdates(0.95)}).full_slice();
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