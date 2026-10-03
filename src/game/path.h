#pragma once

#include "prelude.h"

namespace npath {

struct Path {
    Vec<Vec2> nodes;
    Vec<f32> cumulative_dist;
};

}