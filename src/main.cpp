#include <iostream>

#include "graphics/engine.h"
#include "graphics/input.h"
#include "graphics/data.h"
#include "graphics/ui.h"
#include "game/logic.h"

#include "init_game.h"

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
    if (sprite.is_billboard()) { panic; }
    return glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)), dir - glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(sprite.scale));
}

glm::vec2 raycast(f32 screenX, f32 screenY, Cam const& cam, f32 zIntercept) {
    const f32 epsilon = 0.0001f;
    const glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 rayDir;
    glm::vec3 rayOrigin;
    f32 aspect = f32(data::framebufferWidth) / f32(data::framebufferHeight);
    f32 percentScreenX = screenX / data::framebufferWidth;
    f32 percentScreenY = screenY / data::framebufferHeight;
    f32 offsetScreenX = (0.5f - percentScreenX) * 2.0f;
    f32 offsetScreenY = (0.5f - percentScreenY) * 2.0f;
    glm::vec3 forward = cam.dirLook();
    glm::vec3 right = cam.dirRight();
    glm::vec3 up = glm::cross(forward, right);
    if (cam.orthographic) {
        glm::vec3 offset =
            right * cam.viewWidth * offsetScreenX +
            up * (cam.viewWidth/aspect) * offsetScreenY;
        rayDir = forward;
        rayOrigin = cam.pos + offset;
    } else {
        f32 tanHalfFov = tan(cam.fov * 0.5f);
        rayDir = glm::normalize(
            forward +
            right * (offsetScreenX * tanHalfFov * aspect) +
            up * (offsetScreenY * tanHalfFov)
        );
        rayOrigin = cam.pos;
    }
    f32 denom = glm::dot(normal, rayDir);
    if (abs(denom) > epsilon) {
        f32 t = glm::dot((glm::vec3(0.0f, 0.0f, zIntercept) - rayOrigin), normal) / denom;
        glm::vec3 intercept = rayOrigin + rayDir * t;
        return glm::vec2(intercept.x, intercept.y);
    }
    warn("raycasting into infinity");
    return glm::vec2(cam.pos.x, cam.pos.y);
}

TID findClosestMonkey(glm::vec2 worldPos) {
    Vec2 myPos = { worldPos.x, worldPos.y };
    TID closest = {};
    f32 closestDistSq = F32_INF;
    for (ntower::Tower const& t : *towers) {
        f32 lenSq = (myPos - t.pos).len_sq();
        if (lenSq < closestDistSq) {
            closestDistSq = lenSq;
            closest = t.id;
        }
    }
    return closest;
}

bool mouseCaptured = false;
bool gamePaused = false;
void handleKeyboardInput(Cam& c) {
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
    constexpr u32 keysMonkeyNum = 5;
    u32 keysMonkey[keysMonkeyNum] = { GLFW_KEY_Z, GLFW_KEY_X, GLFW_KEY_C, GLFW_KEY_V, GLFW_KEY_B };
    for (u32 i = 0; i < keysMonkeyNum; i++) {
        if (input::keyPressed(keysMonkey[i])) {
            ui::placementMonkeySelectedIndex = ui::placementMonkeySelectedIndex == i ? U32_MAX : i;
        }
    }
    constexpr u32 keysBloonNum = 5;
    u32 keysBloon[keysBloonNum] = { GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5 };
    for (u32 i = 0; i < keysBloonNum; i++) {
        if (input::keyPressed(keysBloon[i])) {
            bloons->add(nbloon::Bloon::spawn(&bloon_protos[i], &path));
        }
    }
    // unfocus
    if (input::keyPressed(GLFW_KEY_ESCAPE)) {
        glfwSetInputMode(engine::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseCaptured = false;
        ui::placementMonkeySelectedIndex = U32_MAX;
    }
    // delete monkey when selected WARNING duplicate of ui's functionality
    if (ui::selectedMonkey != TID{} && input::keyPressed(GLFW_KEY_DELETE)) {
        remove_tower(ui::selectedMonkey);
        ui::selectedMonkey = TID{};
    }
}

void handleMouseInput(Cam const& c) {
    if (input::mouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        // focus/unfocus window
        if (mouseCaptured) {
            glfwSetInputMode(engine::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mouseCaptured = false;
        } else {
            glfwSetInputMode(engine::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseCaptured = true;
        }
    }
    if (input::mouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        glm::vec2 pos = raycast(input::mousePos.x, input::mousePos.y, c, 0.0f);
        if (ui::placementMonkeySelectedIndex == U32_MAX) {
            TID closestMonkey = findClosestMonkey(pos);
            if (closestMonkey != TID{}) {
                f32 dist = (towers->find(closestMonkey)->pos - Vec2{ pos.x, pos.y }).len();
                if (dist > 1.5f) {
                    ui::selectedMonkey = TID{};
                } else {
                    ui::selectedMonkey = closestMonkey;
                }
            }
        } else {
            towers->add(ntower::Tower::spawn(&tower_protos[ui::placementMonkeySelectedIndex], { pos.x, pos.y }, c.yaw + glm::radians(180.0f)));
            ui::placementMonkeySelectedIndex = U32_MAX;
        }
    }
}

void frameTick(Cam const& c) {
    engine::ShaderUniformData uniformData = {
        .proj = c.projMat(),
        .view = c.viewMat(),
        .camPosition = c.pos,
        .camLookDir = c.dirLook(),
        .isOrtho = c.orthographic
    };
    Vec<engine::ShaderInstanceData> instanceData{ .arena = &frame_arena }; instanceData.reserve(1 + towers->list.size + 1);
    Vec<engine::ShaderInstanceDataBB> instanceDataBB{ .arena = &frame_arena }; instanceDataBB.reserve(bloons->list.size + projs->list.size);
    Vec<P<engine::MeshRef, u32>> meshList{ .arena = &frame_arena }; // limitation: towers, bloons and other may not use the same meshes efficiently; which is fine, really

    // the ground
    instanceData.push({ .model = glm::rotate(glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f)), glm::vec3(20.0f, 20.0f, 1.0f)), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)), .texture = textures.ground });
    meshList.push({ meshes.quad, 1 });

    // sort bloons
    Vec<nbloon::Bloon> bloonsSorted = bloons->list.clone(&frame_arena);
    std::sort(bloonsSorted.begin(), bloonsSorted.end(), [](nbloon::Bloon const& first, nbloon::Bloon const& second) { return first.proto->sprite.mesh < second.proto->sprite.mesh; });
    for (nbloon::Bloon const& b : bloonsSorted) {
        if (b.proto->sprite.is_billboard()) {
            instanceDataBB.push({
                .position = glm::vec3(b.pos.x, b.pos.y, 0.0f),
                .size = 1.5f,
                .texture = b.proto->sprite.texture,
            });
        } else {
            if (meshList.size == 0 || b.proto->sprite.mesh != meshList.back().a) {
                meshList.push({ b.proto->sprite.mesh, 0 });
            }
            meshList.back().b += 1;
            instanceData.push({
                .model = getStaticModelMatrix(b.proto->sprite, b.pos.x, b.pos.y, b.dir, c),
                .texture = b.proto->sprite.texture,
            });
        }
    }

    // towers
    meshList.push({ meshes.monke, 0 });
    for (ntower::Tower& tower : towers->list) {
        meshList.back().b += 1;
        instanceData.push({
            .model = getAnimationModelMatrix(tower.sprite, tower.pos.x, tower.pos.y, tower.dir),
            .texture = tower.sprite.texture
        });
    }
    // preview tower
    if (ui::placementMonkeySelectedIndex != U32_MAX && !ui::captureMouse) {
        meshList.back().b += 1;
        glm::vec2 pos = raycast(input::mousePos.x, input::mousePos.y, c, 0.0f);
        visual::Animated sprite = tower_protos[ui::placementMonkeySelectedIndex].sprite;
        sprite.state = visual::AnimationState::Idle;
        instanceData.push({
            .model = getAnimationModelMatrix(sprite, pos.x, pos.y, c.yaw + glm::radians(180.0f)),
            .texture = sprite.texture
        });
    }
    
    // projectiles
    for (nproj::Projectile& proj : *projs) {
        instanceDataBB.push({
            .position = glm::vec3(proj.pos.x, proj.pos.y, 0.0f),
            .size = 1.0f,
            .texture = proj.proto->sprite.texture,
        });
    }

    engine::startDraw(uniformData);
    engine::startDrawMesh(instanceData.full_slice());
    for(P<engine::MeshRef, u32> const& meshGroup : meshList) {
        if (meshGroup.b == 0) { continue; }
        engine::drawMesh(meshGroup.a, meshGroup.b);
    }
    engine::endDrawMesh();
    engine::drawBillboard(instanceDataBB.full_slice()); // bloons and projectiles
    ui::drawUI();
    engine::endDraw();
}

int main() {
    initTestPrototypes();
	engine::init();
    data::init();
    input::init();
    ui::init();
    Cam c = { .pos = glm::vec3(-5.0f, 0.0f, 1.0f), .pitch = 0, .yaw = 0, .fov = glm::radians(45.0f), .viewWidth = 10.0f, .orthographic = false };
    while (!glfwWindowShouldClose(engine::window)) {
        frame_arena.reset();
        data::frameTick();
        input::frameTick();
        ui::createUI();
        if (!ui::captureMouse) { handleMouseInput(c); }
        if (!ui::captureKeyboard) { handleKeyboardInput(c); }
        frameTick(c);
        //if (!gamePaused && data::timeSinceLastUpdate() >= (1.0 / data::TARGET_UPS)) {
        //while (!gamePaused && data::needDoUpdate()) { // would work perfectly, but alas, makes pausing impossible without jump-aheads on unpause; TODO make a better system later; also prone to death spirals
        for (u32 repeats = 0; !gamePaused && data::needDoUpdate() && repeats < 2; repeats += 1) {
            updateTick();
            data::updateTick();
        }
    }
	engine::cleanup();
}