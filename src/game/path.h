#pragma once

#include "prelude.h"

struct Path {
    Vec<Vec2> nodes;
    Vec<f32> cumulative_dist;
};