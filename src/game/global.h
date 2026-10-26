#pragma once

#include "prelude.h"

//template <typename T>
//struct SMap;

// Prototypes
Vec<nbloon::Proto> bloon_protos;
Vec<nproj::Proto> proj_protos;
Vec<nattack::Proto*> attack_protos;
Vec<ntower::Proto> tower_protos;

// Global Game State
blist::BList* bloons;
plist::PList* projs;
tlist::TList* towers;

i32 hp;
// Map map;
// some more info about current round, round state, how far we're into the round, and so much more
// also like, game state machine that includes paused, in menus, in game, all other menus and stuff idk#pragma once
