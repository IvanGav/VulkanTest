#include <iostream>

#include "graphics/engine.h"
#include "graphics/input.h"
#include "graphics/data.h"
#include "game/attack.h"

struct {
    engine::TextureRef monke;
    engine::TextureRef kyaru;
    engine::TextureRef triangle;
} textures;

struct {
    engine::MeshRef monke;
    engine::MeshRef testQuad;
} meshes;

bool doLogFps = true;
void logFps() {
    if (!doLogFps) { return; }
    static f32 logAt = 1.0f;
    if (data::time >= logAt) {
        std::cout << "fps: " << data::fps << std::endl;
        logAt = data::time + 1.0;
    }
}

// angles in radians
struct Cam {
    glm::vec3 pos;
    f32 pitch;
    f32 yaw;
    f32 fov; // for perspective
    f32 viewWidth; // for orthographic
    bool orthographic;

    glm::mat4 projMat() {
        f32 aspect = f32(data::framebufferWidth) / f32(data::framebufferHeight);
        glm::mat4 proj = orthographic ? 
            glm::ortho(-viewWidth, viewWidth, -viewWidth / aspect, viewWidth / aspect, 0.1f, 1000.0f) :
            glm::perspective(fov, aspect, 0.1f, 1000.0f);
        proj[1][1] *= -1;
        return proj;
    }
    glm::mat4 viewMat() {
        return glm::lookAt(pos, pos + dirLook(), dirUp());
    }
    glm::vec3 dirLook() {
        return glm::normalize(glm::vec3(cos(yaw) * cos(pitch), sin(yaw) * cos(pitch), sin(pitch)));
    }
    glm::vec3 dirForward() {
        return glm::normalize(glm::vec3(cos(yaw), sin(yaw), 0.0f));
    }
    glm::vec3 dirRight() {
        return -glm::normalize(glm::cross(dirForward(), dirUp()));
    }
    glm::vec3 dirUp() {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }
};

f64 animationStartTime = -F64_INF;
const f64 animationLength = 0.3;
void beginAnimation() {
    animationStartTime = data::time;
}
glm::mat4 getAnimationModelMatrix() {
    if (data::time - animationStartTime > animationLength) {
        return glm::rotate(glm::mat4(1.0f), f32(data::time) * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    f64 animationProgress = (data::time - animationStartTime) / animationLength; // 0 to 1
    animationProgress *= 2.0;
    animationProgress = animationProgress > 1.0 ? (-(animationProgress - 2.0)) : animationProgress;
    return glm::rotate(
        glm::rotate(glm::mat4(1.0f), f32(data::time) * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        f32(animationProgress) * glm::radians(75.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
}

bool mouseCaptured = false;
void handleInput(Cam& c) {
    if (input::keyDown(GLFW_KEY_W)) {
        c.pos += c.dirForward() * 0.2f * f32(data::timeDelta * 100.0);
    }
    if (input::keyDown(GLFW_KEY_S)) {
        c.pos -= c.dirForward() * 0.2f * f32(data::timeDelta * 100.0);
    }
    if (input::keyDown(GLFW_KEY_A)) {
        c.pos += c.dirRight() * 0.2f * f32(data::timeDelta * 100.0);
    }
    if (input::keyDown(GLFW_KEY_D)) {
        c.pos -= c.dirRight() * 0.2f * f32(data::timeDelta * 100.0);
    }
    if (input::keyDown(GLFW_KEY_SPACE)) {
        c.pos += c.dirUp() * 0.2f * f32(data::timeDelta * 100.0);
    }
    if (input::keyDown(GLFW_KEY_LEFT_SHIFT)) {
        c.pos -= c.dirUp() * 0.2f * f32(data::timeDelta * 100.0);
    }
    if (mouseCaptured) {
        c.yaw -= f32(input::mouseDelta.x / 500.0);
        c.pitch -= f32(input::mouseDelta.y / 500.0);
        c.pitch = glm::clamp(c.pitch, glm::radians(-89.0f), glm::radians(89.0f));
    }
    if (input::scrollDelta.y != 0.0) {
        if (c.orthographic) {
            c.viewWidth -= input::scrollDelta.y / 2.0;
            c.viewWidth = glm::max(c.viewWidth, 0.0f);
        }
        else {
            c.fov -= (f32)glm::radians(input::scrollDelta.y * 2.0);
            c.fov = glm::clamp(c.fov, glm::radians(1.0f), glm::radians(179.0f));
        }
    }
    if (input::keyPressed(GLFW_KEY_Q)) {
        c.orthographic = !c.orthographic;
    }
    if (input::keyPressed(GLFW_KEY_E)) {
        beginAnimation();
    }
}

void captureReleaseMouse() {
    if (input::keyPressed(GLFW_KEY_ESCAPE)) {
        glfwSetInputMode(engine::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseCaptured = false;
    }
    if (input::mouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        glfwSetInputMode(engine::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
    }
}

Vec<nbloon::Proto> bloonProtos;
Vec<nproj::Proto> projProtos;
blist::BList bloons;
plist::PList projs;
npath::Path path;
void synthesizeBloonState() {
    projProtos.push(nproj::Proto{
        .damage = 1,
        .pierce = 3,
        .move = nmove::Move{
            .type = nmove::Type::MoveSimple,
            .move_simple = nmove::MoveSimple{ .speed = 0.2 }
        },
        .hitbox = hitbox::HB{
            .type = hitbox::Type::Circle,
            .circle = hitbox::Circle{ .r = 0.5 }
        },
        .lifetime_ticks = 100
    });
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
        .children = { .data = &bloonProtos.data, .size = 1 },
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
        .children = { .data = b2children, .size = 3 },
        .hitbox = hitbox::HB{.type = hitbox::Type::Circle, .circle = hitbox::Circle{.r = 0.5}},
        .max_hitbox_dist = 0.5,
        .speed_status_immune = false
        });
    path = npath::Path{ .nodes = Vec<Vec2>::with(&global_arena, Vec2{ -5.0, -5.0 }, Vec2{ 5.0, 5.0 }, Vec2{ -5.0, 6.0 }), .cumulative_dist = Vec<f32>::with(&global_arena, 0.0f, 14.1421356237f, 24.1920112448f) };
    bloons = blist::BList::create();
    projs = plist::PList::create();
}

int main() {
    {
        textures.monke = engine::loadTexture("asset/monke.png");
        textures.kyaru = engine::loadTexture("asset/kyaru.png");
        textures.triangle = engine::loadTexture("asset/triangle.png");
    }
    {
        meshes.monke = engine::loadMeshObj("asset/monke.obj");
        meshes.testQuad = engine::loadTestMesh();
    }
	engine::init();
    data::init();
    input::init();
    synthesizeBloonState();
    Cam c = { .pos = glm::vec3(-5.0f, 0.0f, 1.0f), .pitch = 0, .yaw = 0, .fov = glm::radians(45.0f), .viewWidth = 10.0f, .orthographic = false };
    while (!glfwWindowShouldClose(engine::window)) {
        data::frameTick();
        input::frameTick();
        captureReleaseMouse();
        handleInput(c);
        engine::ShaderUniformData uniformData = {
                .proj = c.projMat(),
                .view = c.viewMat(),
                .camPosition = c.pos
        };
        std::vector<engine::ShaderInstanceData> instanceData = {
            { .model = getAnimationModelMatrix(), .texture = textures.monke },
            { .model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, sin(f32(data::time)) * 5.0f)), .texture = textures.triangle }
        };
        for (nbloon::Bloon& bloon : bloons) {
            instanceData.push_back({
                .model = glm::rotate(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(bloon.pos.x, bloon.pos.y, 0.0f)), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)), bloon.dir, glm::vec3(1.0f, 0.0f, 0.0f)),
                .texture = (bloon.type.has_any({1}) ? textures.triangle : textures.kyaru)
            });
        }
        for (nproj::Projectile& proj: projs) {
            instanceData.push_back({
                .model = glm::rotate(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(proj.pos.x, proj.pos.y, 0.0f)), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)), proj.dir, glm::vec3(1.0f, 0.0f, 0.0f)),
                .texture = textures.monke
            });
        }
        engine::startDraw(uniformData, { .data = instanceData.data(), .size = (u32)(instanceData.size())});
        engine::drawMesh(meshes.monke, 2);
        engine::drawMesh(meshes.testQuad, bloons.bloons.size + projs.list.size);
        engine::endDraw();
        logFps();
        if (input::keyPressed(GLFW_KEY_1)) {
            projs.add(nproj::Projectile::spawn(&projProtos[0], ref(nproj::Buff{}), { c.pos.x, c.pos.y }, c.yaw));
        }
        if (input::keyPressed(GLFW_KEY_2)) {
            bloons.add(nbloon::Bloon::spawn(&bloonProtos[0], &path));
        }
        if (input::keyPressed(GLFW_KEY_3)) {
            bloons.add(nbloon::Bloon::spawn(&bloonProtos[1], &path));
        }
        if (input::keyPressed(GLFW_KEY_4)) {
            bloons.add(nbloon::Bloon::spawn(&bloonProtos[2], &path));
        }
        if (input::keyDown(GLFW_KEY_Z)) {
            for (nbloon::Bloon& bloon : bloons.bloons)
                bloon.move();
        }
        if (input::keyDown(GLFW_KEY_X)) {
            for (nproj::Projectile& proj : projs.list)
                proj.move();
        }
        std::vector<nbloon::BID> poppedBloons; poppedBloons.reserve(128);
        for (nbloon::Bloon& bloon : bloons) {
            for (u32 pi = 0; pi < projs.list.size; pi++) {
                nproj::Projectile& proj = projs.list[pi];
                if (proj.pierce > 0 && bloon.proto->hitbox.intersect(&proj.proto->hitbox, bloon.pos, proj.pos, bloon.dir, proj.dir) && projs.can_hit(pi, bloon.bft)) {
                    u32 oldHp = bloon.hp;
                    bloon.hp -= proj.proto->damage;
                    proj.pierce -= 1;
                    projs.record_hit(pi, bloon.bft);
                    if (bloon.hp <= 0 && oldHp > 0) { poppedBloons.push_back(bloon.bid); }
                }
            }
        }
        for (nbloon::BID bid : poppedBloons) {
            bloons.pop(bid);
        }
        for (u32 i = 0; i < projs.list.size; i++) {
            if (projs.list[i].pierce == 0 || projs.list[i].lifetime_ticks == 0) {
                projs.del(i); i--;
            }
        }
    }
	engine::cleanup();
}