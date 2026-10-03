#pragma once

#include "prelude.h"

namespace hitbox {
    struct HB;

    struct Circle {
        f32 r;
    };
    struct Ellipse {
        f32 r1;
        f32 r2;
    };
    struct Composite {
        Slice<P<Vec2, HB>> hb;
    };
    // https://math.stackexchange.com/questions/1114879/detect-if-two-ellipses-intersect
    // https://www.geometrictools.com/Documentation/IntersectionOfEllipses.pdf
    enum class Type {
        Circle, Ellipse, Composite
    };
    struct HB {
        Type type;
        union {
            Circle circle;
            Ellipse ellipse;
            Composite composite;
        };

        static HB make_circle(f32 r) { return HB{ .type = Type::Circle, .circle = Circle { .r = r }}; }

        bool intersect(HB const* other, Vec2 pos, Vec2 pos_other, f32 dir, f32 dir_other) const {
            switch(this->type) {
                case Type::Circle: {
                    switch(this->type) {
                        case Type::Circle: { f32 dist = this->circle.r + other->circle.r; return (pos-pos_other).len_sq() < dist*dist; }
                        case Type::Ellipse: panic;
                        case Type::Composite: { return other->intersect(this, pos_other, pos, dir_other, dir); }
                    }
                }
                case Type::Ellipse: {
                    switch(this->type) {
                        case Type::Circle: panic;
                        case Type::Ellipse: panic;
                        case Type::Composite: { return other->intersect(this, pos_other, pos, dir_other, dir); }
                    }
                }
                case Type::Composite: {
                    for(u32 i = 0; i < this->composite.hb.size; i++) {
                        Vec2 pos_off = this->composite.hb[i].a.rotate(dir);
                        // if(this->composite.hb[i].b.intersect(other, pos + pos_off, pos_other, dir, dir_other)) {
                        if(other->intersect(&this->composite.hb[i].b, pos_other, pos + pos_off, dir_other, dir)) {
                            return true;
                        }
                    }
                    return false;
                }
            }
        }
    };
}