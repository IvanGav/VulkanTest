#include <iostream>

#include "graphics/engine.h"
#include "graphics/input.h"
#include "graphics/data.h"
#include "game/logic.h"

#include "init_game.h"

f64 logAt = 1.0;
void logFps() {
    if (data::time >= logAt) {
        std::cout << "fps: " << data::fps << ", ups: " << data::ups << std::endl;
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

    glm::mat4 projMat() const {
        f32 aspect = f32(data::framebufferWidth) / f32(data::framebufferHeight);
        glm::mat4 proj = orthographic ? 
            glm::ortho(-viewWidth, viewWidth, -viewWidth / aspect, viewWidth / aspect, 0.1f, 1000.0f) :
            glm::perspective(fov, aspect, 0.1f, 1000.0f);
        proj[1][1] *= -1;
        return proj;
    }
    glm::mat4 viewMat() const {
        return glm::lookAt(pos, pos + dirLook(), dirUp());
    }
    glm::vec3 dirLook() const {
        return glm::normalize(glm::vec3(cos(yaw) * cos(pitch), sin(yaw) * cos(pitch), sin(pitch)));
    }
    glm::vec3 dirForward() const {
        return glm::normalize(glm::vec3(cos(yaw), sin(yaw), 0.0f));
    }
    glm::vec3 dirRight() const {
        return -glm::normalize(glm::cross(dirForward(), dirUp()));
    }
    glm::vec3 dirUp() const {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }
};

glm::mat4 getAnimationModelMatrix(visual::Animated const& animation, f32 x, f32 y, f32 dir) {
    f64 animationProgress = animation.animation_progress();
    if (animation.state == visual::AnimationState::Idle) {
        return glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)), dir + glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    if (animation.state == visual::AnimationState::Spawn) {
        f32 moveUp = (1.0f - animationProgress) * 5;
        return glm::translate(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)), dir + glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.0f, 0.0f, moveUp));
    }
    animationProgress *= 2.0;
    animationProgress = animationProgress > 1.0 ? (-(animationProgress - 2.0)) : animationProgress;
    return glm::rotate(
        glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)), dir + glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        f32(animationProgress) * glm::radians(75.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
}

glm::mat4 getStaticModelMatrix(visual::Static const& sprite, f32 x, f32 y, f32 dir, Cam const& cam) {
    if (sprite.is_billboard()) {
        return glm::rotate(glm::rotate(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)), cam.yaw, glm::vec3(0.0f, 0.0f, 1.0f)), -cam.pitch + glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    } else {
        return glm::rotate(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)), dir, glm::vec3(1.0f, 0.0f, 0.0f));
    }
}

bool mouseCaptured = false;
bool gamePaused = false;
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
    if (input::keyPressed(GLFW_KEY_P)) {
        gamePaused = !gamePaused;
    }
    if (input::keyPressed(GLFW_KEY_Q)) {
        c.orthographic = !c.orthographic;
    }
    if (input::keyPressed(GLFW_KEY_E)) {
        for (ntower::Tower& tower : *towers) {
            std::cout << "tower id {" << tower.id.i << "," << tower.id.gen << "} has " << tower.pop_count << " pops" << std::endl;
        }
        std::cout << "______________" << std::endl;
    }
    /* gameplay related inputs */
    constexpr u32 keysMonkeyNum = 2;
    u32 keysMonkey[keysMonkeyNum] = { GLFW_KEY_Z, GLFW_KEY_X };
    for (u32 i = 0; i < keysMonkeyNum; i++) {
        if (input::keyPressed(keysMonkey[i])) {
            towers->add(ntower::Tower::spawn(&tower_protos[i], { c.pos.x, c.pos.y }));
        }
    }
    constexpr u32 keysBloonNum = 4;
    u32 keysBloon[keysBloonNum] = { GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4 };
    for (u32 i = 0; i < keysBloonNum; i++) {
        if (input::keyPressed(keysBloon[i])) {
            bloons->add(nbloon::Bloon::spawn(&bloon_protos[i], &path));
        }
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

void frameTick(Cam const& c) {
    engine::ShaderUniformData uniformData = {
        .proj = c.projMat(),
        .view = c.viewMat(),
        .camPosition = c.pos
    };
    Vec<engine::ShaderInstanceData> instanceData{ .arena = &frame_arena }; instanceData.reserve(1 + bloons->list.size + projs->list.size + towers->list.size);
    instanceData.push({ .model = glm::rotate(glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f)), glm::vec3(20.0f, 20.0f, 1.0f)), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)), .texture = textures.ground });
    for (nbloon::Bloon& bloon : *bloons) {
        instanceData.push({
            .model = getStaticModelMatrix(bloon.proto->sprite, bloon.pos.x, bloon.pos.y, bloon.dir, c),
            .texture = bloon.proto->sprite.texture,
            });
    }
    for (nproj::Projectile& proj : *projs) {
        instanceData.push({
            .model = getStaticModelMatrix(proj.proto->sprite, proj.pos.x, proj.pos.y, proj.dir, c),
            .texture = proj.proto->sprite.texture,
            });
    }
    for (ntower::Tower& tower : towers->list) {
        instanceData.push({
            .model = getAnimationModelMatrix(tower.sprite, tower.pos.x, tower.pos.y, tower.dir),
            .texture = tower.sprite.texture
            });
    }
    engine::startDraw(uniformData, { .data = instanceData.data, .size = instanceData.size });
    engine::drawMesh(meshes.quad, 1); // ground
    engine::drawMesh(meshes.quad, bloons->list.size + projs->list.size); // bloons and projectiles
    engine::drawMesh(meshes.monke, towers->list.size); // monke
    engine::endDraw();
}

int main() {
    printd(std::numeric_limits<float>::is_iec559);
    initTestPrototypes();
	engine::init();
    data::init();
    input::init();
    Cam c = { .pos = glm::vec3(-5.0f, 0.0f, 1.0f), .pitch = 0, .yaw = 0, .fov = glm::radians(45.0f), .viewWidth = 10.0f, .orthographic = false };
    while (!glfwWindowShouldClose(engine::window)) {
        frame_arena.reset();
        data::frameTick();
        input::frameTick();
        captureReleaseMouse();
        handleInput(c);
        frameTick(c);
        if (!gamePaused && data::timeSinceLastUpdate() >= (1.0 / data::TARGET_UPS)) {
            updateTick();
            data::updateTick();
        }
        logFps();
    }
	engine::cleanup();
}