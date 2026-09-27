#include <iostream>

#include "engine.h"
#include "input.h"
#include "data.h"

struct {
    engine::TextureRef monke;
    engine::TextureRef kyaru;
    engine::TextureRef triangle;
} textures;

struct {
    engine::MeshRef monke;
    engine::MeshRef testQuad;
} meshes;

bool doLogFps = false;
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
    f32 fov;

    glm::mat4 projMat() {
        glm::mat4 proj = glm::perspective(fov, engine::swapchainExtent.width / (f32)engine::swapchainExtent.height, 0.1f, 1000.0f);
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

bool mouseCaptured = false;
void cameraMovement(Cam& c) {
    if (input::keyDown(GLFW_KEY_W)) {
        c.pos += c.dirForward() * 0.2f;
    }
    if (input::keyDown(GLFW_KEY_S)) {
        c.pos -= c.dirForward() * 0.2f;
    }
    if (input::keyDown(GLFW_KEY_A)) {
        c.pos += c.dirRight() * 0.2f;
    }
    if (input::keyDown(GLFW_KEY_D)) {
        c.pos -= c.dirRight() * 0.2f;
    }
    if (input::keyDown(GLFW_KEY_SPACE)) {
        c.pos += c.dirUp() * 0.2f;
    }
    if (input::keyDown(GLFW_KEY_LEFT_ALT)) {
        c.pos -= c.dirUp() * 0.2f;
    }
    if (mouseCaptured) {
        c.yaw -= f32(input::mouseDelta.x / 500.0);
        c.pitch -= f32(input::mouseDelta.y / 500.0);
        c.pitch = glm::clamp(c.pitch, glm::radians(-89.0f), glm::radians(89.0f));
    }
    if (input::scrollDelta.y != 0.0) {
        c.fov -= (f32)glm::radians(input::scrollDelta.y * 2.0);
        c.fov = glm::clamp(c.fov, glm::radians(1.0f), glm::radians(179.0f));
    }
    if (input::keyPressed(GLFW_KEY_F)) {
        doLogFps = !doLogFps;
    }
}

void captureReleaseMouse() {
    if (input::keyDown(GLFW_KEY_ESCAPE)) {
        glfwSetInputMode(engine::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseCaptured = false;
    }
    if (glfwGetMouseButton(engine::window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        glfwSetInputMode(engine::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
    }
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
    Cam c = { .pos = glm::vec3(-5.0f, 0.0f, 1.0f), .pitch = 0, .yaw = 0, .fov = glm::radians(45.0f) };
    while (!glfwWindowShouldClose(engine::window)) {
        data::frameTick();
        input::frameTick();
        captureReleaseMouse();
        cameraMovement(c);
        engine::ShaderUniformData uniformData = {
                .proj = c.projMat(),
                .view = c.viewMat(),
                .camPosition = c.pos
        };
        engine::ShaderInstanceData instanceData[3] = {
            { .model = glm::rotate(glm::mat4(1.0f), f32(data::time) * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)), .texture = textures.monke },
            { .model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, sin(f32(data::time)) * 5.0f)), .texture = textures.triangle },
            { .model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -10.0f, sin(f32(data::time) + 3.0f) * 3.0f)), .texture = textures.kyaru }
        };
        engine::startDraw(uniformData, { .data = instanceData, .size = 3 });
        engine::drawMesh(meshes.monke, 2);
        engine::drawMesh(meshes.testQuad, 1);
        engine::endDraw();
        logFps();
    }
	engine::cleanup();
}